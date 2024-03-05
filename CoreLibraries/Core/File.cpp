#include "./File.h"

#include "./MappedFile.h"
#include <Ty/Defer.h>
#include <Ty/IOVec.h>
#include <Ty/Try.h>
#include <unistd.h> // _SC_IOV_MAX

namespace Core {

constexpr File::File(int fd, bool should_close)
    : m_fd(fd)
    , m_should_close(should_close)
{
}

File::~File() {
    if (is_valid()) {
        flush().ignore();
        if (m_should_close)
            close();
        invalidate();
    }
}

void File::close() const
{
    System::close(m_fd).ignore();
}

ErrorOr<File> File::open_for_writing(StringView path, u32 flags, mode_t mode)
{
    auto path_buffer = TRY(StringBuffer::create_fill(path, "\0"sv));
    return open_for_writing(path_buffer.view().data(), flags, mode);
}

ErrorOr<File> File::open_for_writing(c_string path, u32 flags, mode_t mode)
{
    auto fd = TRY(System::open(path, flags|O_WRONLY, mode));
    return File(fd, true);
}

File File::from(int fd, bool should_close)
{
    return File(fd, should_close);
}

ErrorOr<void> File::flush()
{
    if (m_buffer.size() == 0)
        return {};
    TRY(System::write(m_fd, m_buffer.data(), m_buffer.size()));
    m_buffer.clear();
    return {};
}

ErrorOr<u32> File::write(void const* data, usize size)
{
    return TRY(write(StringView::from_parts((char*)data, size)));
}

ErrorOr<u32> File::write(StringView string)
{
    if (m_fd == STDERR_FILENO) {
        return TRY(System::write(m_fd, string.data(), string.size()));
    }
    if (string.size() > m_buffer.size_left()) {
        TRY(flush());
    }
    return TRY(m_buffer.write(string));
}

bool File::is_tty() const
{
    return System::isatty(m_fd);
}

File& File::stdout()
{
    thread_local static auto file = File::from(STDOUT_FILENO, false);
    return file;
}

File& File::stderr()
{
    thread_local static auto file = File::from(STDERR_FILENO, false);
    return file;
}

}
