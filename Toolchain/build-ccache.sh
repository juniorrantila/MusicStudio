#!/bin/bash

set -e

BASE_PATH=`realpath $(dirname $0)`
INSTALL_DIR="$BASE_PATH/Tools"
BUILD_DIR="$BASE_PATH/Build/ccache"
VERSION="v4.10.2"
REPO="https://github.com/ccache/ccache"
REPO_DIR="$BASE_PATH/Vendor/ccache"
DONE="$BASE_PATH/Done"

source $BASE_PATH/env

[ -f "$DONE/ccache" ] && exit

if ! [ -s "$REPO_DIR" ]; then
    git clone --depth 1 -b $VERSION $REPO $REPO_DIR
fi

mkdir -p $DONE
mkdir -p $BUILD_DIR
mkdir -p $INSTALL_DIR/bin

pushd $BUILD_DIR
trap popd EXIT

cmake -S $REPO_DIR    \
    -DCMAKE_BUILD_TYPE=Release \
    -GNinja \
    --install-prefix=$INSTALL_DIR
ninja install

touch $DONE/ccache
