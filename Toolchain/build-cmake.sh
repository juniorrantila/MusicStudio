#!/bin/bash

set -e

BASE_PATH=`realpath $(dirname $0)`
INSTALL_DIR="$BASE_PATH/Tools"
BUILD_DIR="$BASE_PATH/Build/CMake"
VERSION="v3.29.4"
REPO="https://github.com/Kitware/CMake"
REPO_DIR="$BASE_PATH/Vendor/CMake"
DONE="$BASE_PATH/Done"

source $BASE_PATH/env

[ -f "$DONE/cmake" ] && exit

if ! [ -s "$REPO_DIR" ]; then
    git clone --depth 1 -b $VERSION $REPO $REPO_DIR
fi

mkdir -p $DONE
mkdir -p $BUILD_DIR
mkdir -p $INSTALL_DIR

pushd $BUILD_DIR
trap popd EXIT
$REPO_DIR/bootstrap       \
    --no-system-libs      \
    --generator=Ninja     \
    --parallel=`nproc`    \
    --prefix=$INSTALL_DIR
ninja install

touch $DONE/cmake
