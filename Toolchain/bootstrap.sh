#!/bin/bash

set -e

BASE_PATH=`realpath $(dirname $0)`
TOOLCHAIN_DIR="$BASE_PATH"

"$TOOLCHAIN_DIR/build-odin.sh"
"$TOOLCHAIN_DIR/build-python.sh"
"$TOOLCHAIN_DIR/build-ninja.sh"
"$TOOLCHAIN_DIR/build-cmake.sh"
"$TOOLCHAIN_DIR/build-ccache.sh"
"$TOOLCHAIN_DIR/build-llvm.sh"
