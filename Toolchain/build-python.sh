#!/bin/bash

set -e

BASE_PATH=`realpath $(dirname $0)`
INSTALL_DIR="$BASE_PATH/Tools"
BUILD_DIR="$BASE_PATH/Build/cpython"
VERSION="3.13"
REPO="https://github.com/python/cpython"
REPO_DIR="$BASE_PATH/Vendor/cpython"
DONE="$BASE_PATH/Done"

source $BASE_PATH/env

[ -f "$DONE/python" ] && exit

if ! [ -s "$REPO_DIR" ]; then
    git clone --depth 1 -b $VERSION $REPO $REPO_DIR
fi

mkdir -p $DONE
mkdir -p $BUILD_DIR
mkdir -p $INSTALL_DIR/bin

pushd $BUILD_DIR
trap popd EXIT

$REPO_DIR/configure \
    --disable-test-modules \
    --prefix $INSTALL_DIR \

make -j$(nproc) install

touch $DONE/python
