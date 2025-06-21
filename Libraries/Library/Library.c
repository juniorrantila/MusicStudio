#include "./Library.h"

#include <FS/FSVolume.h>
#include <Ty/Verify.h>

#include <dlfcn.h>
#include <mach-o/dyld.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

Library* library_create_from_memory(c_string identifier, void* mem, usize size, e_library* error)
{
    NSObjectFileImageReturnCode rc;

    NSObjectFileImage img;
    rc = NSCreateObjectFileImageFromMemory(mem, size, &img);
    if (rc != NSObjectFileImageSuccess) {
      if (error) *error = e_library_could_not_create_object;
      return 0;
    }

    NSModule mod = NSLinkModule(img, identifier, NSLINKMODULE_OPTION_PRIVATE);
    if (!mod) {
        if (error) *error = e_library_could_not_link;
        return 0;
    }
    NSDestroyObjectFileImage(img);

    return (Library*)mod;
}


Library* library_create_from_path(c_string path, e_library* error)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        if (error) *error = e_library_could_not_open;
        return 0;
    }
    struct stat st;
    if (fstat(fd, &st) < 0) {
        if (error) *error = e_library_could_not_stat;
        close(fd);
        return 0;
    }
    void* mem = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mem == (void*)-1) {
        if (error) *error = e_library_could_not_map;
        close(fd);
        return 0;
    }

    Library* lib = library_create_from_memory(path, mem, st.st_size, error);
    munmap(mem, st.st_size);
    close(fd);
    return lib;
}

void library_destroy(Library* library)
{
    NSUnLinkModule((NSModule)library, NSUNLINKMODULE_OPTION_NONE);
    free(library);
}

void* library_get_symbol(Library const* library, c_string name)
{
    if (!library) return 0;
    NSSymbol symbol = NSLookupSymbolInModule((NSModule)library, name);
    if (!symbol) return 0;
    return NSAddressOfSymbol(symbol);
}

c_string library_strerror(e_library error)
{
    switch (error) {
#define LIBRARY_ERROR(ident, pretty, ...) case e_library_##ident: return pretty;
#include "./Error.def"
#undef LIBRARY_ERROR
    }
    return "(unknown error)";
}
