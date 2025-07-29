#include <Core/MappedFile.h>
#include <Main/Main.h>
#include <Tar/Tar.h>
#include <CLI/CLIParser.h>
#include <stdio.h>

ErrorOr<int> Main::main(int argc, char const* argv[])
{
    CLIParser argument_parser;
    cli_parser_init(&argument_parser, nullptr);

    c_string tar_path = nullptr;
    argument_parser.add_positional_argument("path"s, "tar-path"s, (void*)&tar_path, [](CLIParser*, void* user, c_string arg) -> cli_error {
        *((c_string*)user) = arg;
        return nullptr;
    });

    if (c_string error = argument_parser.run(argc, argv); error != nullptr) {
        (void)fprintf(stderr, "%s", error);
        return 1;
    }

    auto file = TRY(Core::MappedFile::open(tar_path));
    auto tar = tar_init(file.data(), file.size());

    char const* path = nullptr;
    u64 path_size = 0;
    u8 const* content = nullptr;
    u64 content_size = 0;
    while (untar(&tar, &path, &path_size, &content, &content_size))
        printf("%.*s\n", (int)path_size, path);

    return 0;
}
