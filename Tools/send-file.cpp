#include <LibCLI/ArgumentParser.h>
#include <LibMain/Main.h>
#include <Basic/Context.h>

#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/signal.h>

ErrorOr<int> Main::main(int argc, c_string argv[])
{
    init_default_context("sendfile");

    auto argument_parser = CLI::ArgumentParser();

    c_string from = nullptr;
    TRY(argument_parser.add_positional_argument("from", [&](c_string arg) {
        from = arg;
    }));

    c_string to = nullptr;
    TRY(argument_parser.add_positional_argument("from", [&](c_string arg) {
        to = arg;
    }));

    if (auto result = argument_parser.run(argc, argv); result.is_error()) {
        TRY(result.error().show());
        return 1;
    }

    int from_fd = open(from, O_RDONLY);
    if (from_fd < 0) return Error::from_string_literal_with_errno("could not open 'from'");

    struct stat st;
    if (fstat(from_fd, &st) < 0) return Error::from_string_literal_with_errno("could not stat 'from'");

    void* bytes = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, from_fd, 0);
    if (bytes == MAP_FAILED) return Error::from_string_literal_with_errno("could not map 'from'");

    int to_fd = open(to, O_WRONLY|O_TRUNC|O_CREAT, st.st_mode);
    if (to_fd < 0) return Error::from_string_literal_with_errno("could not open 'to'");

    if (write(to_fd, bytes, st.st_size) < 0) return Error::from_string_literal_with_errno("could not copy file content");

    fsync(to_fd);
    close(to_fd);
    return 0;
}
