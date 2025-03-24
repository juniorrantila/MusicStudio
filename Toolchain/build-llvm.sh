#!/bin/bash

set -e

BASE_PATH=`realpath $(dirname $0)`
INSTALL_DIR="$BASE_PATH/Tools"
BUILD_DIR="$BASE_PATH/Build/llvm"
VERSION="release/20.x"
REPO="https://github.com/llvm/llvm-project"
REPO_DIR="$BASE_PATH/Vendor/llvm"
DONE="$BASE_PATH/Done"

source $BASE_PATH/env

[ -f "$DONE/llvm" ] && exit

if ! [ -s "$REPO_DIR" ]; then
    git clone --depth 1 -b $VERSION $REPO $REPO_DIR
fi

mkdir -p $DONE
mkdir -p $BUILD_DIR
mkdir -p $INSTALL_DIR/bin

pushd $BUILD_DIR
trap popd EXIT

if [ $(uname -s) = "Darwin" ]
then
    DEFAULT_SYSROOT="-DDEFAULT_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk"
fi

cmake -S $REPO_DIR/llvm                                     \
    -DCMAKE_BUILD_TYPE=Release                              \
    -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra;lld"    \
    -DLLVM_ENABLE_RUNTIMES="compiler-rt"                    \
    -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR                     \
    -DLLVM_TARGETS_TO_BUILD="AArch64;X86;WebAssembly"       \
    -DLLVM_INCLUDE_TESTS=NO                                 \
    -DLLVM_INCLUDE_EXAMPLES=NO                              \
    -DLLVM_ENABLE_BACKTRACES=NO                             \
    -DLLVM_INCLUDE_BENCHMARKS=NO                            \
    -DLLVM_CCACHE_BUILD=YES                                 \
    -DLLDB_USE_SYSTEM_DEBUGSERVER=ON                        \
    "$DEFAULT_SYSROOT"                                      \
    -GNinja                                                 \

ninja install

touch $DONE/llvm
