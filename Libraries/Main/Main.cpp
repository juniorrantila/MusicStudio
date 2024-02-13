#include "Main.h"
#include "Core/File.h"

int main(int argc, c_string argv[])
{
    auto result = Main::main(argc, argv);
    if (result.is_error()) {
        Core::File::stderr().writeln(result.error()).ignore();
        return 1;
    }
    return result.value();
}
