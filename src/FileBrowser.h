#pragma once
#include "./Forward.h"
#include "./Widget.h"

#include <Ty/Forward.h>
#include <Ty/StringBuffer.h>
#include <UI/Forward.h>

#include <Rexim/File.h>
#include <Rexim/LA.h>

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

struct FileBrowser : Widget {
    static ErrorOr<FileBrowser> create(StringView path);
    virtual ~FileBrowser() = default;

    FileBrowser(FileBrowser&& other)
        : m_files(other.m_files)
        , m_cursor(other.m_cursor)
        , m_dir_path(other.m_dir_path)
        , m_file_path(other.m_file_path)
        , m_hovered_file(move(other.m_hovered_file))
    {
    }
    FileBrowser& operator=(FileBrowser&& other)
    {
        if (this == &other)
            return *this;

        m_files = other.m_files;
        m_cursor = other.m_cursor;
        m_dir_path = other.m_dir_path;
        m_file_path = other.m_file_path;
        m_hovered_file = move(other.m_hovered_file);
        return *this;
    }

    ErrorOr<void> open_dir(StringView path);
    ErrorOr<void> change_dir();
    Optional<StringView> current_file();

    void render(UI::UI&, EventLoop& event_loop, Vec4f box) override;

private:
    constexpr FileBrowser() = default;

    Files m_files {};
    usize m_cursor { 0 };
    String_Builder m_dir_path {};
    String_Builder m_file_path {};
    StringView m_hovered_file {};
};
