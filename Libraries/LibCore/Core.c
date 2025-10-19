#include "./Core.h"

#include <Basic/Verify.h>

#include <errno.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

C_API KError core_read_entire_file(c_string path, char const** out_data, u64* out_size, Allocator* allocator)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0)
        return kerror_unix(errno);

    struct stat st = {};
    if (fstat(fd, &st) < 0) {
        close(fd);
        return kerror_unix(errno);
    }

    if (st.st_size == 0) {
        *out_size = 0;
        *out_data = nullptr;
        return kerror_none;
    }

    char* buf = (char*)memalloc(allocator, st.st_size, 1);
    if (!buf) return kerror_unix(ENOMEM);
    ssize_t rv = read(fd, buf, st.st_size);
    if (rv < 0) return kerror_unix(errno);
    VERIFY(rv == st.st_size);

    close(fd);
    *out_data = buf;
    *out_size = st.st_size;
    return kerror_none;
}

C_API KError core_write_entire_file(c_string path, void const* data, u64 size)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0)
        return kerror_unix(errno);

    if (write(fd, data, size) < 0) {
        close(fd);
        return kerror_unix(errno);
    }

    return kerror_none;
}
