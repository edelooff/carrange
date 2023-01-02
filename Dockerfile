FROM alpine:latest

RUN apk update \
  && apk upgrade \
  && apk add --no-cache \
    clang \
    clang-dev \
    alpine-sdk

COPY composer.cpp example.in.txt example.out.txt Makefile ./

RUN make test

ENTRYPOINT ["./composer"]
