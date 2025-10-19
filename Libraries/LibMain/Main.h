#pragma once
#include <LibTy/Base.h>
#include <LibTy/ErrorOr.h>

namespace Main {

ErrorOr<int> main(int argc, c_string argv[]);

}

int main(int argc, c_string argv[]);
