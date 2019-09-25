FROM alpine
COPY . /
RUN chmod a+x /installer.sh
RUN ./installer.sh dev.env
RUN chmod a+x /mytool.sh
RUN rm dev.env
CMD [ "/bin/sh","-c","/mytool.sh" ]