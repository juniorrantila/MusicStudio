#!/bin/bash

set -e

BASE_PATH=`realpath $(dirname $0)`
INSTALL_DIR="$BASE_PATH/Tools"
BUILD_DIR="$BASE_PATH/Build/lldbui"
VERSION="main"
REPO="https://github.com/juniorrantila/lldbui"
REPO_DIR="$BASE_PATH/Vendor/lldbui"
DONE="$BASE_PATH/Done"

source $BASE_PATH/env

[ -f "$DONE/lldbui" ] && exit

if ! [ -s "$REPO_DIR" ]; then
    git clone --depth 1 -b $VERSION $REPO $REPO_DIR
fi

mkdir -p $DONE
mkdir -p $BUILD_DIR
mkdir -p $INSTALL_DIR/bin

pushd $REPO_DIR
trap popd EXIT

cargo build --release --target-dir $BUILD_DIR
cp $BUILD_DIR/release/lldbui $INSTALL_DIR/bin
touch $DONE/lldbui
