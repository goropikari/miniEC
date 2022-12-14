FROM mcr.microsoft.com/vscode/devcontainers/base:ubuntu-22.04

ARG REMOTE_USER=vscode
ARG QMK_VERSION=0.18.17

RUN apt-get update && \
    apt-get upgrade -y && \
    apt-get install -y git python3-pip make gcc-avr binutils-avr avr-libc avrdude vim gcc-arm-none-eabi ripgrep

WORKDIR /home/vscode
USER vscode
ENV PATH=$PATH:/home/vscode/.local/bin
RUN python3 -m pip install qmk

RUN git clone -b $QMK_VERSION --recurse-submodules https://github.com/qmk/qmk_firmware.git

WORKDIR /home/vscode/qmk_firmware
