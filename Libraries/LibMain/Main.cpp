#include "Main.h"
#include <LibCore/File.h>

#include <Basic/Context.h>

#include <sys/syslimits.h>
#include <stdlib.h>
#include <libgen.h>

int main(int argc, c_string argv[])
{
    static c_string program_name;
    static char buf[PATH_MAX];
    if (!program_name) {
        basename_r(getenv("_"), buf);
        program_name = buf;
    }
    init_default_context(program_name);

    auto result = Main::main(argc, argv);
    if (result.is_error()) {
        Core::File::stderr().writeln(result.error()).ignore();
        return 1;
    }
    return result.value();
}
