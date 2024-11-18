#include "./PluginManager.h"
#include "Library/Library.h"
#include "Plugin.h"
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>

namespace MS {

void PluginManager::deinit()
{
    for (usize i = 0; i < plugin_count; i++) {
        plugins[i].deinit();
        library_destroy(libraries[i]);
    }
    plugin_count = 0;
}

ErrorOr<Id<Plugin>> PluginManager::instantiate(c_string path)
{
    auto file = mapped_files.find(StringView::from_c_string(path));
    if (file.has_value()) {
        auto file_id = file.value();
        void* data = mapped_files[file_id].data();
        usize size = mapped_files[file_id].size();

        e_library error;
        Library* lib = library_create_from_memory(path, data, size, &error);
        if (!lib) {
            return Error::from_string_literal(library_strerror(error));
        }
        auto id = plugin_count++;
        libraries[id] = lib;
        plugins[id] = Plugin(id, this, lib);
        return Id<Plugin>(id);
    }

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        return Error::from_errno();
    }
    struct stat st;
    if (fstat(fd, &st) < 0) {
        return Error::from_errno();
    }
    void* mem = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mem == (void*)-1) {
        close(fd);
        return Error::from_errno();
    }
    TRY(mapped_files.append(StringView::from_c_string(path), View<u8>((u8*)mem, st.st_size)));
    e_library error;
    auto* lib = library_create_from_path(path, &error);
    if (!lib) {
        return Error::from_string_literal(library_strerror(error));
    }
    auto id = plugin_count++;
    libraries[id] = lib;
    plugins[id] = Plugin(id, this, lib);
    return Id<Plugin>(id);
}

}
