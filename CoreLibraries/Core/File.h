#pragma once
#ifdef stderr
#pragma push_macro("stderr")
#define stderr stderr
#endif

#ifdef stdout
#pragma push_macro ("stdout")
#define stdout stdout
#endif

#include <Ty/System.h>
#include <Ty/Traits.h>
#include <Ty/ErrorOr.h>
#include <Ty/Forward.h>
#include <Ty/IOVec.h>

namespace Core {

struct MappedFile;

struct File {
    File(File const&) = delete;
    File& operator=(File const&) = delete;

    constexpr File(File&& other)
        : m_buffer(move(other.m_buffer))
        , m_fd(other.m_fd)
        , m_should_close(other.m_should_close)
    {
        other.invalidate();
    }

    constexpr File& operator=(File&& other)
    {
        if (this == &other)
            return *this;
        m_buffer = move(other.m_buffer);
        m_fd = other.m_fd;
        m_should_close = other.m_should_close;
        other.invalidate();
        return *this;
    }

    ~File();

    static File& stdout();
    static File& stderr();

    static File from(int fd, bool should_close);
    static ErrorOr<File> open_for_writing(StringView path, u32 flags = 0, mode_t mode = 0666);
    static ErrorOr<File> open_for_writing(c_string path, u32 flags = 0, mode_t mode = 0666);

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
            written += TRY(results[i]);

        return written;
    }

    template <typename... Args>
    constexpr ErrorOr<u32> writeln(Args const&... args)
    {
        return TRY(write(args..., "\n"sv));
    }

    ErrorOr<u32> write(void const* data, usize size);
    ErrorOr<u32> write(StringView string);

    template <typename T>
    ErrorOr<u32> write(T const& value)
    {
        return TRY(Formatter<T>::write(*this, value));
    }

    bool is_tty() const;

    ErrorOr<void> flush();

private:
    constexpr File(int fd, bool should_close);
    ErrorOr<void> buffer(StringView);

    void close() const;
    constexpr bool is_valid() const { return m_fd != -1; }
    constexpr void invalidate() { m_fd = -1; }

    StringBuffer m_buffer {};
    int m_fd { -1 };
    bool m_should_close { false };
};

}

#ifdef stdout
#pragma pop_macro("stdout")
#endif

#ifdef stderr
#pragma pop_macro("stderr")
#endif
