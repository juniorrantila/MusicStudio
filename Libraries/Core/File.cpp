#include "File.h"
#include "System.h"
#include <Ty/Defer.h>
#include <Ty/IOVec.h>
#include <Ty/Try.h>
#include <unistd.h> // _SC_IOV_MAX

namespace Core {

void File::close() const { System::close(m_fd).ignore(); }

ErrorOr<File> File::open_for_writing(StringView path, mode_t mode)
{
    auto path_buffer = TRY(StringBuffer::create_fill(path, "\0"sv));
    return open_for_writing(path_buffer.view().data, mode);
}

ErrorOr<File> File::open_for_writing(c_string path, mode_t mode)
{
    auto fd = TRY(Core::System::open(path, O_WRONLY, mode));
    return File(fd, true);
}

ErrorOr<usize> File::writev_max_count()
{
    static isize max_count = 0;
    if (static bool first = true; first) [[unlikely]] {
        max_count = TRY(Core::System::sysconf(_SC_IOV_MAX));
        first = false;
    }
    return max_count;
}

namespace {
constexpr auto min(auto a, auto b) { return a < b ? a : b; }
}

ErrorOr<usize> File::nonatomic_writev(IOVec const* iovec,
    usize count) const
{
    usize total = 0;
    auto batch_size = TRY(writev_max_count());
    for (usize i = 0; i < count; i += batch_size) {
        auto size = (int)min(count - i, batch_size);
        auto bytes_written
            = TRY(Core::System::writev(m_fd, &iovec[i], size));
        if (__builtin_add_overflow(total, bytes_written, &total))
            return Error::from_string_literal("overflow");
    }
    return total;
}

}
