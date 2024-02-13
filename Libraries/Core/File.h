#pragma once
#include "System.h"
#include "Ty/Traits.h"
#include <Ty/ErrorOr.h>
#include <Ty/Forward.h>
#include <Ty/IOVec.h>

namespace Core {

struct MappedFile;

struct File {
    static File& stdout()
    {
        thread_local static auto file
            = File::from(STDOUT_FILENO, false);
        return file;
    }

    static File& stderr()
    {
        thread_local static auto file
            = File::from(STDERR_FILENO, false);
        return file;
    }

    static ErrorOr<File> open_for_writing(StringView path,
        mode_t mode = 0666);
    static ErrorOr<File> open_for_writing(c_string path,
        mode_t mode = 0666);
    static File from(int fd, bool should_close)
    {
        return File(fd, should_close);
    }

    constexpr File(File&& other)
        : m_fd(other.m_fd)
    {
        other.invalidate();
    }

    constexpr File& operator=(File&& other)
    {
        m_fd = other.m_fd;
        other.invalidate();
        return *this;
    }

    ~File()
    {
        if (is_valid()) {
            (void)flush();
            if (m_should_close)
                close();
            invalidate();
        }
    }

    void close() const;

    ErrorOr<usize> nonatomic_writev(IOVec const*,
        usize count) const;

    template <typename... Args>
    constexpr ErrorOr<u32> write(Args const&... args) requires(
        sizeof...(Args) > 1)
    {
        constexpr auto args_size = sizeof...(Args);
        ErrorOr<u32> results[args_size] = {
            write(args)...,
        };
        u32 written = 0;
        for (u32 i = 0; i < args_size; i++)
            written += TRY(move(results[i]));

        if (m_fd == STDERR_FILENO)
            TRY(flush());

        return written;
    }

    template <typename... Args>
    constexpr ErrorOr<u32> writeln(Args... args)
    {
        return TRY(write(args..., "\n"sv));
    }

    ErrorOr<u32> write(void const* data, usize size)
    {
        return TRY(buffer_or_write(
            StringView { (c_string)data, (u32)size }));
    }

    ErrorOr<u32> write(StringView string)
    {
        return TRY(buffer_or_write(string));
    }

    template <typename T>
    requires is_trivially_copyable<T> ErrorOr<u32> write(T value)
    {
        return TRY(Formatter<T>::write(*this, value));
    }

    template <typename T>
    requires(!is_trivially_copyable<T>) ErrorOr<u32> write(
        T const& value)
    {
        return TRY(Formatter<T>::write(*this, value));
    }

    ErrorOr<void> flush()
    {
        TRY(Core::System::write(m_fd, buffer.view()));
        buffer.clear();

        return {};
    }

    bool is_tty() const { return Core::System::isatty(m_fd); }

private:
    constexpr File() = default;

    constexpr ErrorOr<usize> buffer_or_write(StringView string)
    {
        if (buffer.capacity() <= string.size)
            return TRY(Core::System::write(m_fd, string));
        if (buffer.size_left() <= string.size)
            TRY(flush());
        return TRY(buffer.write(string));
    }

    constexpr ErrorOr<usize> buffer_or_write(u64 number)
    {
        constexpr auto max_characters_in_u64 = 20;
        if (buffer.size_left() <= max_characters_in_u64)
            TRY(flush());
        return TRY(buffer.write(number));
    }

    constexpr ErrorOr<usize> buffer_or_write(Error error)
    {
        auto written
            = TRY(write(error.function(), ": "sv, error.message()));
        written += TRY(writeln(" ["sv, error.file(), ":"sv,
            error.line_in_file(), "]"sv));
        return written;
    }

    static ErrorOr<usize> writev_max_count();

    constexpr File(int fd, bool should_close)
        : m_fd(fd)
        , m_should_close(should_close)
    {
    }

    constexpr bool is_valid() const { return m_fd >= 0; }
    constexpr void invalidate() { m_fd = 0; }

    StringBuffer buffer {};
    int m_fd { -1 };
    bool m_should_close { false };
};

}
