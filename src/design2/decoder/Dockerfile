# Dockerfile for the 2025 eCTF
# Make any changes here to set up your build environment (e.g., installing crypto
# libraries, dependencies, the compiler for a different language)

FROM ubuntu:24.04

LABEL version="0.2"
LABEL description="Example Decoder Docker Container for the 2025 eCTF"

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
    git \
    wget \
    libtool \
    texinfo \
    unzip    

# Install MSDK
RUN apt-get install -y unzip && \
    wget https://github.com/analogdevicesinc/msdk/archive/refs/tags/v2024_02.zip && \
    unzip v2024_02.zip && \
    rm -f v2024_02.zip

ENV MAXIM_PATH=/root/msdk-2024_02

WORKDIR /decoder

ENTRYPOINT ["bash", "-c", "make inc/global.secrets.h && make release DECODER_ID=${DECODER_ID} && cp build/max78000.elf build/max78000.bin /out"]

# Sample run command:
# docker run --rm -v ./:/decoder -v ./../global.secrets:/global.secrets:ro -v ./build_out:/out -e DECODER_ID=0xdeadbeef decoder

# venv
# . ..\.venv\Scripts\Activate.ps1

# windows
# docker run --rm -v .\:/decoder -v .\..\global.secrets:/global.secrets:ro -v .\build_out:/out -e DECODER_ID=0xdeadbeef decoder

#openocd
# ./openocd.exe -s scripts/ -f interface/cmsis-dap.cfg -f target/max78000.cfg -c "bindto 0.0.0.0; init"

# connect gdb
# docker run --rm -it -p 3333:3333/tcp -v .\build_out:/out --workdir=/root --entrypoint /bin/bash decoder -c " cp -r /out/* /root/ && gdb-multiarch max78000.elf "

# target remote host.docker.internal:3333

# subscribe
# python -m ectf25.tv.subscribe subscription.bin COM3

#flash 
# python -m ectf25.utils.flash .\build_out\max78000.bin COM3

# test
# python -m ectf25.utils.tester --port COM3 -s .\global.secrets\secrets.json rand -c 1 -f 64

#gen sub
# python -m ectf25_design.gen_subscription global.secrets ../subscription1.bin 0xDEADBEEF 0 18446744073709551615 1