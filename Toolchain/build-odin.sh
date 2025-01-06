#!/bin/bash

set -e

BASE_PATH=`realpath $(dirname $0)`
INSTALL_DIR="$BASE_PATH/Tools"
VERSION="b136aa26c8269c08c94380578dad0c2dcdcbfec1"
REPO="https://github.com/odin-lang/Odin"
REPO_DIR="$BASE_PATH/Vendor/odin"
DONE="$BASE_PATH/Done"

[ -f "$DONE/odin" ] && exit

if ! [ -s "$REPO_DIR" ]; then
    git clone $REPO $REPO_DIR
    git -C $REPO_DIR checkout $VERSION
fi

mkdir -p $DONE
mkdir -p $INSTALL_DIR/bin

pushd $REPO_DIR
trap popd EXIT

./build_odin.sh release-native

mv odin $INSTALL_DIR/bin/odin

touch $DONE/odin
