#pragma once
#include <Ty/StringBuffer.h>

struct ChangePathEvent {
    mutable StringBuffer file_path_buf;

    c_string file_path() const
    {
        if (file_path_buf.data()[file_path_buf.size() - 1] != '\0') {
            MUST(file_path_buf.write("\0"sv));
        }
        return file_path_buf.data();
    }
};
