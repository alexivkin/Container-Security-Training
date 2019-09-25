// Busybox elevate privileges
// Compile: musl-gcc -static -Os uproot.c -o uproot
// chmod ug+s uproot

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

int main(int argc, char *argv[]){
    char *env[]={"PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin",NULL};
    setreuid(geteuid(),geteuid());
    if (argc == 1){
        // printf("Starting sh\n");
        execle("/bin/busybox","sh",(const char*)NULL,env);
    } else {
        // printf("Starting %s\n",argv[1]);
        execvpe("/bin/busybox",argv+1,env);
    }
    perror("rootup");
    return 1;
}
