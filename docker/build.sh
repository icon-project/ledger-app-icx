#!/bin/bash

SCRIPT_DIR=$(cd `dirname "$0"` ; pwd)
NAME="nanos-dev"
VERSION="latest"

docker build -t "${NAME}:${VERSION}" ${SCRIPT_DIR}
