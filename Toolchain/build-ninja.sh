#!/bin/bash

set -e

BASE_PATH=`realpath $(dirname $0)`
INSTALL_DIR="$BASE_PATH/Tools"
BUILD_DIR="$BASE_PATH/Build/ninja"
VERSION="v1.12.1"
REPO="https://github.com/ninja-build/ninja"
REPO_DIR="$BASE_PATH/Vendor/ninja"
DONE="$BASE_PATH/Done"

source $BASE_PATH/env

[ -f "$DONE/ninja" ] && exit

if ! [ -s "$REPO_DIR" ]; then
    git clone --depth 1 -b $VERSION $REPO $REPO_DIR
fi

mkdir -p $DONE
mkdir -p $BUILD_DIR
mkdir -p $INSTALL_DIR/bin

pushd $BUILD_DIR
trap popd EXIT

$REPO_DIR/configure.py --bootstrap
cp $BUILD_DIR/ninja $INSTALL_DIR/bin

touch $DONE/ninja
