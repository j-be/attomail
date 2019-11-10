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

COPY femtomail.c /femtomail/
COPY Makefile /femtomail/

################################################################################
## Build
##

ENV USERNAME="status"
ENV CC="arm-linux-gnueabihf-gcc"

WORKDIR /femtomail/
ENTRYPOINT make clean && make femtomail
