#include "./File.h"

#ifdef _WIN32
#    include "./minirent.h"
#else
#    include <dirent.h>
#    include <sys/types.h>
#    include <sys/stat.h>
#    include <unistd.h>
#endif // _WIN32
#include "./Util.h"

#include <stdio.h>

Errno read_entire_dir(c_string dir_path, Files* files)
{
    Errno result = 0;
    DIR *dir = NULL;
    struct dirent* ent = NULL;

    dir = opendir(dir_path);
    if (dir == NULL) {
        return_defer(errno);
    }

    errno = 0;
    ent = readdir(dir);
    while (ent != NULL) {
        File_Type type = FT_OTHER;
        switch (ent->d_type) {
            case DT_REG:
                type = FT_REGULAR;
                break;

            case DT_DIR:
                type = FT_DIRECTORY;
                break;
        }
        File file = (File) {
            .name = temp_strdup(ent->d_name),
            .type = type,
        };
        // Skip hidden files and current path.
        if (ent->d_name[0] == '.' && ent->d_name[1] != '.') {
        } else {
            da_append(files, file);
        }
        ent = readdir(dir);
    }

    if (errno != 0) {
        return_defer(errno);
    }

defer:
    if (dir) closedir(dir);
    return result;
}

Errno write_entire_file(const char *file_path, const char *buf, usize buf_size)
{
    Errno result = 0;
    FILE *f = NULL;

    f = fopen(file_path, "wb");
    if (f == NULL) return_defer(errno);

    fwrite(buf, 1, buf_size, f);
    if (ferror(f)) return_defer(errno);

defer:
    if (f) fclose(f);
    return result;
}

static Errno file_size(FILE *file, usize *size)
{
    long saved = ftell(file);
    if (saved < 0) return errno;
    if (fseek(file, 0, SEEK_END) < 0) return errno;
    long result = ftell(file);
    if (result < 0) return errno;
    if (fseek(file, saved, SEEK_SET) < 0) return errno;
    *size = (usize) result;
    return 0;
}

Errno read_entire_file(const char *file_path, String_Builder *sb)
{
    Errno result = 0;
    FILE *f = NULL;
    usize size = 0;
    Errno err = 0;

    f = fopen(file_path, "r");
    if (f == NULL) return_defer(errno);

    err = file_size(f, &size);
    if (err != 0) return_defer(err);

    if (sb->capacity < size) {
        sb->capacity = size;
        sb->items = (char*)realloc(sb->items, sb->capacity*sizeof(*sb->items));
        assert(sb->items != NULL && "Buy more RAM lol");
    }

    fread(sb->items, size, 1, f);
    if (ferror(f)) return_defer(errno);
    sb->count = size;

defer:
    if (f) fclose(f);
    return result;
}

Errno type_of_file(const char *file_path, File_Type *ft)
{
#ifdef _WIN32
#error "TODO: type_of_file() is not implemented for Windows"
#else
    struct stat sb = {};
    if (stat(file_path, &sb) < 0) return errno;
    if (S_ISREG(sb.st_mode)) {
        *ft = FT_REGULAR;
    } else if (S_ISDIR(sb.st_mode)) {
        *ft = FT_DIRECTORY;
    } else {
        *ft = FT_OTHER;
    }
#endif
    return 0;
}
