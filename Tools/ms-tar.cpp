#include <CLI/ArgumentParser.h>
#include <Main/Main.h>
#include <Tar/Tar.h>
#include <stdio.h>

namespace Main {

ErrorOr<int> main(int argc, char const* argv[])
{
    auto argument_parser = CLI::ArgumentParser();

    if (auto result = argument_parser.run(argc, argv); result.is_error()) {
        TRY(result.error().show());
        return 1;
    }

    Tar* tar = tar_create();
    if (!tar) {
        return Error::from_string_literal("could not create tar");
    }

    // if (tar_add(tar, "bar", "bar\n", 4) < 0) {
    //     return Error::from_string_literal("could not add bar");
    // }

    if (tar_add(tar, "foo", "foo file\n", 9) < 0) {
        return Error::from_string_literal("could not add foo");
    }

    static char zeros[512];
    if (tar_add(tar, "zero", zeros, 512) < 0) {
        return Error::from_string_literal("could not add foo");
    }

    TarFile file = tar_as_file(tar);
    fwrite(file.bytes, file.size, 1, stdout);

    return 0;
}

}
