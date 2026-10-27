# Build environment for the robot firmware: Arm GCC, host C++ compiler and
# Pico C SDK 2.2.0. The repository is mounted at /work at run time; see README.
FROM ubuntu:24.04

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      ca-certificates git make g++ gcc-arm-none-eabi libnewlib-arm-none-eabi \
 && rm -rf /var/lib/apt/lists/*

ENV PICO_SDK_PATH=/opt/pico-sdk
RUN git clone --depth 1 --branch 2.2.0 https://github.com/raspberrypi/pico-sdk.git "$PICO_SDK_PATH" \
 && git -C "$PICO_SDK_PATH" submodule update --init --depth 1 lib/tinyusb lib/cyw43-driver lib/lwip

WORKDIR /work
