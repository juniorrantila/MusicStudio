#include "./StringBuilder.h"
#include "./Util.h"

typedef enum {
    FT_REGULAR,
    FT_DIRECTORY,
    FT_OTHER,
} File_Type;

typedef struct {
    const char *name;
    File_Type type;
} File;

typedef struct {
    File* items;
    usize count;
    usize capacity;
} Files;

Errno type_of_file(c_string file_path, File_Type *ft);
Errno read_entire_file(c_string file_path, String_Builder *sb);
Errno write_entire_file(c_string file_path, const char *buf, usize buf_size);
Errno read_entire_dir(c_string dir_path, Files *files);
