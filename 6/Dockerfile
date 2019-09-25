FROM golang:latest as builder
WORKDIR /httpsrv
COPY *.go .
RUN CGO_ENABLED=0 GOARCH=amd64 GOOS=linux go build -tags netgo -installsuffix netgo

FROM scratch
WORKDIR /
COPY --from=builder /httpsrv/httpsrv .
ENTRYPOINT ["/httpsrv"]
