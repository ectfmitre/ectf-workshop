# Dockerfile for the 2025 eCTF
# Make any changes here to set up your build environment (e.g., installing crypto
# libraries, dependencies, the compiler for a different language)

FROM ubuntu:24.04

LABEL version="0.3"
LABEL description="Decoder Docker Container for the 2025 eCTF"

ARG DEBIAN_FRONTEND=noninteractive
WORKDIR /root

# Install Requisite Packages
# do this first because it takes the longest
RUN apt-get update && apt-get upgrade -y && apt-get install -y \
    gdb \
    gdb-multiarch \
    gcc-arm-none-eabi \
    binutils-arm-none-eabi \
    make \
    python3 \
    python3-pip \
    python3-venv \
    git \
    wget \
    libtool \
    texinfo \
    unzip

# Install MSDK
RUN wget https://github.com/analogdevicesinc/msdk/archive/refs/tags/v2024_10.zip && \
    unzip v2024_10.zip && \
    rm -f v2024_10.zip
ENV MAXIM_PATH=/root/msdk-2024_10

# Install wolfssl/wolfcrypt
RUN wget https://github.com/wolfSSL/wolfssl/archive/refs/tags/v5.7.6-stable.zip && \
    unzip v5.7.6-stable.zip && \
    rm -f v5.7.6-stable.zip && \
    ln -sf wolfssl-5.7.6-stable wolfssl-stable

# Create Python virtual environment in /root/.venv
RUN python3 -m venv .venv --prompt ectf

# Install Python crypto library
RUN . .venv/bin/activate && python3 -m pip install pycryptodome loguru

# Create directory for generated code (/root/gencode)
RUN mkdir gencode

WORKDIR /decoder

ENTRYPOINT ["bash", "-c", "source /root/.venv/bin/activate && python3 py/codegen.py ${DECODER_ID} && make release DECODER_ID=${DECODER_ID} DEBUG_MODE=${DEBUG_MODE} && cp build/max78000.elf build/max78000.bin /out"]

# Sample run command (during development):
# docker run --rm -v ./decoder:/decoder -v ./global.secrets:/global.secrets:ro -v ./deadbeef_build:/out -e DECODER_ID=0xdeadbeef -e DEBUG_MODE=1 build-decoder
# To poke around the docker environment interactively:
# docker run --rm -it --entrypoint /bin/bash build-decoder
