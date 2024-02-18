#pragma once
#include <Ty/ErrorOr.h>
#include <Ty/Formatter.h>
#include <Ty/StringView.h>

namespace Core {

struct MappedFile {
    enum Mode {
        ModeRead,
        ModeWrite,
        ModeReadWrite,
    };

    MappedFile(MappedFile&& other)
        : m_data(other.m_data)
        , m_size(other.m_size)
        , m_fd(other.m_fd)
    {
        other.invalidate();
    }

    MappedFile& operator=(MappedFile&& other)
    {
        if (&other == this)
            return *this;
        this->~MappedFile();

        m_data = other.m_data;
        m_size = other.m_size;
        m_fd = other.m_fd;
        other.invalidate();
        return *this;
    }

    static ErrorOr<MappedFile> open(StringView path, Mode mode = ModeRead);
    static ErrorOr<MappedFile> open(c_string path, Mode mode = ModeRead);
    ~MappedFile();

    StringView view() const { return StringView((char*)m_data, m_size); }

    u8* data() const { return m_data; }
    u64 size() const { return m_size; }

    bool is_valid() const { return m_data != nullptr; }
    void invalidate() { m_data = nullptr; }

private:
    constexpr MappedFile(u8* data, u32 size, int fd)
        : m_data(data)
        , m_size(size)
        , m_fd(fd)
    {
    }

    constexpr MappedFile() = default;

    u8* m_data;
    u64 m_size;
    int m_fd;
};

}

namespace Ty {

template <>
struct Formatter<Core::MappedFile> {
    template <typename U>
    requires Writable<U>
    static constexpr ErrorOr<u32> write(U& to,
        Core::MappedFile const& file)
    {
        return TRY(to.write(to, file.view()));
    }
};

}
