FROM mcr.microsoft.com/vscode/devcontainers/base:ubuntu-22.04

ARG REMOTE_USER=vscode
ARG QMK_VERSION=0.22.10

RUN apt-get update && \
    apt-get upgrade -y && \
    apt-get install -y git python3-pip make gcc-avr binutils-avr avr-libc avrdude vim gcc-arm-none-eabi ripgrep wget libc6-i386 && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

RUN cd /usr/bin/ && \
    wget https://www.pjrc.com/teensy/hid_listen && \
    chmod +x hid_listen

WORKDIR /home/vscode
USER vscode
ENV PATH=$PATH:/home/vscode/.local/bin
RUN python3 -m pip install qmk

RUN git clone -b $QMK_VERSION --recurse-submodules https://github.com/qmk/qmk_firmware.git

WORKDIR /home/vscode/qmk_firmware
