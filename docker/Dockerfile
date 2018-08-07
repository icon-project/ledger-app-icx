FROM ubuntu:16.04
RUN apt update && apt -y install build-essential libc6-i386 libc6-dev-i386 python python-pip libudev-dev libusb-1.0-0-dev python3-dev git
ENV BOLOS_ENV /work/bolos
RUN mkdir -p ${BOLOS_ENV}
WORKDIR ${BOLOS_ENV}
ADD clang+llvm-4.0.0-x86_64-linux-gnu-ubuntu-16.04.tar.xz ${BOLOS_ENV}
RUN ln -s "clang+llvm-4.0.0-x86_64-linux-gnu-ubuntu-16.04" "clang-arm-fropi"
ADD gcc-arm-none-eabi-5_3-2016q1-20160330-linux.tar.bz2 ${BOLOS_ENV}
ARG BOLOS_SDK_NAME=nanos-secure-sdk-nanos-1421
ADD ${BOLOS_SDK_NAME}.* ${BOLOS_ENV}
ENV BOLOS_SDK ${BOLOS_ENV}/${BOLOS_SDK_NAME}
RUN git clone https://github.com/LedgerHQ/blue-loader-python.git && ( cd blue-loader-python ; pip install ledgerblue )
RUN rm -rf blue-loader-python
CMD /bin/bash
