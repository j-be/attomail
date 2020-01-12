FROM ubuntu:18.04

################################################################################
## Toolchain and libs
##

# Install toolchain
RUN apt-get update
RUN apt-get install -y crossbuild-essential-armhf

################################################################################
## Copy sources
##

COPY attomail.c /attomail/
COPY Makefile /attomail/

################################################################################
## Build
##

ENV USERNAME="status"
ENV CC="arm-linux-gnueabihf-gcc"

WORKDIR /attomail/
ENTRYPOINT make clean && make attomail
