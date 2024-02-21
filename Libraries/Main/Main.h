#pragma once
#include <Ty/Base.h>
#include <Ty/ErrorOr.h>

namespace Main {

ErrorOr<int> main(int argc, c_string argv[]);

}

extern "C" int main(int argc, c_string argv[]);
