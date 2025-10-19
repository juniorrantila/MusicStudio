#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

#include "./Library.h"

#include <Basic/Verify.h>
#include <Basic/Context.h>

#include <dlfcn.h>
#include <mach-o/dyld.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

Library* library_create_from_memory(c_string identifier, void* mem, u64 size)
{
    NSObjectFileImageReturnCode rc;

    NSObjectFileImage img;
    rc = NSCreateObjectFileImageFromMemory(mem, size, &img);
    if (rc != NSObjectFileImageSuccess) {
        errorf("could not create library object");
        return nullptr;
    }

    NSModule mod = NSLinkModule(img, identifier, NSLINKMODULE_OPTION_PRIVATE);
    if (!mod) {
        errorf("could not link library object");
        return nullptr;
    }
    NSDestroyObjectFileImage(img);

    return (Library*)mod;
}


Library* library_create_from_path(c_string path)
{
    Library* lib = nullptr;

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        errorf("could not open '%s': %s", path, strerror(errno));
        goto fi_0;
    }
    struct stat st;
    if (fstat(fd, &st) < 0) {
        errorf("could not stat '%s': %s", path, strerror(errno));
        goto fi_1;
    }
    void* mem = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mem == (void*)-1) {
        errorf("could not map file '%s': %s", path, strerror(errno));
        goto fi_2;
    }

    lib = library_create_from_memory(path, mem, st.st_size);
    munmap(mem, st.st_size);
fi_2:
fi_1:
    close(fd);
fi_0:
    return lib;
}

void library_destroy(Library* library)
{
    NSUnLinkModule((NSModule)library, NSUNLINKMODULE_OPTION_NONE);
}

void* library_get_symbol(Library const* library, c_string name)
{
    if (!library) return 0;
    NSSymbol symbol = NSLookupSymbolInModule((NSModule)library, name);
    if (!symbol) return 0;
    return NSAddressOfSymbol(symbol);
}

#pragma clang diagnostic pop
