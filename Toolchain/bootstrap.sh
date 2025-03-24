#!/bin/bash

set -e

BASE_PATH=`realpath $(dirname $0)`
TOOLCHAIN_DIR="$BASE_PATH"

which python > /dev/null || "$TOOLCHAIN_DIR/build-python.sh"
"$TOOLCHAIN_DIR/build-ninja.sh"
which cmake > /dev/null || "$TOOLCHAIN_DIR/build-cmake.sh"
which ccache > /dev/null || "$TOOLCHAIN_DIR/build-ccache.sh"
"$TOOLCHAIN_DIR/build-llvm.sh"
