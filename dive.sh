#!/bin/bash

IMAGE="nanos-dev:latest"
SCRIPT_DIR="$(cd `dirname ${0}` ; pwd)"

declare -a ARGS
ARGS=()
ARGS+=("-it" "--rm")
ARGS+=("-v" "${SCRIPT_DIR}:${SCRIPT_DIR}" -w "$(pwd)")
ARGS+=("-v" "/dev:/dev" "--privileged")
ARGS+=("$IMAGE" "$@")


docker run "${ARGS[@]}"
