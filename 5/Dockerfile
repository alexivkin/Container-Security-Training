FROM alpine:latest
RUN apk --no-cache add python py-lxml && rm -rf /var/cache/apk/* 
ADD dsvw.py /dsvw.py
CMD ["python", "/dsvw.py"]