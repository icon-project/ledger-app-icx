# Installation Guide

## Platform Requirements
* Docker
* 64bit Linux in virtual or real machine for install.

If you want to just build binary, docker is enough. But if you want to upload
this binary with python tools, then you should have virtual machine with USB
device support or real machine.

## Build Nano-S Application

### Prepare docker image
If you already have docker image named `nanos-dev`, then you may skip this
sequence, but if you don't, then Simply run the script in `docker` directory.

    # cd docker
    # ./build.sh

It will build docker image named `nanos-dev`. It may takes several minutes

### Compiling sources in the container.
Now, you can build your own application in docker.

    # ./dive.sh make

And if it doesn't meet any errors, then it makes binary as `bin/app.elf`.

## Install Nano-S Application
To upload your application on the device, you should have virtual machine
or real machine with 64bit Linux. If you use virtual machine, then please
make sure that it can access your Nano-S device.

### 1. Change virtual machine or real machine udev rule.

Before you plug your device to the computer. You should add new rule
for the device.
Here is content of `/etc/udev/rules.d/80-nanos.rules` file.

    SUBSYSTEMS=="usb", ATTRS{idVendor}=="2c97", ATTRS{idProduct}=="0000", MODE="0666"
    SUBSYSTEMS=="usb", ATTRS{idVendor}=="2c97", ATTRS{idProduct}=="0001", MODE="0666"

To make the file, you should have administrator permission. Please use
`sudo` command for it.

### 2. Prepare device.

Prepare your Nano-S device, and connect it to the devie. And make it show
application selection menu, **DashBoard** application after some steps.
After successful detection you may confirm it with `lsusb` command.

    # lsusb
    Bus 001 Device 001: ID 1d6b:0002 Linux Foundation 2.0 root hub
    Bus 002 Device 021: ID 2c97:0001
    Bus 002 Device 002: ID 80ee:0021 VirtualBox USB Tablet
    Bus 002 Device 001: ID 1d6b:0001 Linux Foundation 1.1 root hub

You may see the device with the ID, **`2c97`**.

### 3. Install application

Now you can install application with simple commands.

    # ./dive.sh make load

If you want to remove application

    #./dive.sh make delete


