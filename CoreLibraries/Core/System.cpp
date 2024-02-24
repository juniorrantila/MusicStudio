#include "System.h"
#include "Syscall.h"
#include "Ty/Defer.h"
#include "Ty/StringBuffer.h"
#include <sys/stat.h>
#include <unistd.h> // sysconf()
#include <fcntl.h>

namespace Core::System {

ErrorOr<void> fsync(int fd)
{
    auto rv = syscall(Syscall::fsync, fd);
    if (rv < 0)
        return Error::from_syscall(rv);
    return {};
}

#if __APPLE__ && __x86_64__
struct [[gnu::packed]] St {
    i32 dev;
    i32 inode;
    i16 mode;
    u16 links;
    u32 uid;
    u32 gid;
    i32 rdev;
    struct timespec access_time;
    struct timespec modify_time;
    struct timespec change_time;
    i64 size;
    i64 blocks;
    i32 block_size;
    u32 flags;
    u32 gen;
    i32 lspare;
    i64 qspare[2];
};
#elif __linux__ && __x86_64__
struct St {
    u64 dev;
    u64 inode;
    u64 links;
    u32 mode;
    u32 uid;
    u32 gid;
    u64 rdev;
    i64 size;
    i64 block_size;
    i64 blocks;
    struct timespec access_time;
    struct timespec modify_time;
    struct timespec change_time;
    i64 __glibc_reserved[3];
    u64 __glibc_reserved4;
    u64 __glibc_reserved5;
};
#else
#    warning "unimplemented"
#endif

ErrorOr<Stat> fstat(int fd)
{
    St st {};
    auto rv = syscall(Syscall::fstat, fd, &st);
    if (rv < 0)
        return Error::from_syscall(rv);
    struct stat stat;
    __builtin_memset(&stat, 0, sizeof(stat));
    stat.st_dev = st.dev;
    stat.st_ino = st.inode;
    stat.st_mode = st.mode;
    stat.st_nlink = st.links;
    stat.st_uid = st.uid;
    stat.st_gid = st.gid;
    stat.st_rdev = st.rdev;
    stat.st_size = st.size;
    stat.st_blksize = st.block_size;
    stat.st_blocks = st.blocks;
#if __APPLE__
    stat.st_atimespec = st.access_time;
    stat.st_mtimespec = st.modify_time;
    stat.st_ctimespec = st.change_time;
#elif __linux__
    stat.st_atim = st.access_time;
    stat.st_mtim = st.modify_time;
    stat.st_ctim = st.change_time;
#else
#    warning "unimplemented"
#endif
    return Stat(stat);
}

ErrorOr<Stat> stat(c_string path)
{
    St st {};
    auto rv = syscall(Syscall::stat, path, &st);
    if (rv < 0)
        return Error::from_syscall(rv);
    struct stat stat;
    __builtin_memset(&stat, 0, sizeof(stat));
    stat.st_dev = st.dev;
    stat.st_ino = st.inode;
    stat.st_mode = st.mode;
    stat.st_nlink = st.links;
    stat.st_uid = st.uid;
    stat.st_gid = st.gid;
    stat.st_rdev = st.rdev;
    stat.st_size = st.size;
    stat.st_blksize = st.block_size;
    stat.st_blocks = st.blocks;
#if __APPLE__
    stat.st_atimespec = st.access_time;
    stat.st_mtimespec = st.modify_time;
    stat.st_ctimespec = st.change_time;
#elif __linux__
    stat.st_atim = st.access_time;
    stat.st_mtim = st.modify_time;
    stat.st_ctim = st.change_time;
#else
#    warning "unimplemented"
#endif
    return Stat(stat);
}

ErrorOr<usize> write(int fd, void const* data, usize size)
{
    auto rv = syscall(Syscall::write, fd, data, size);
    if (rv < 0)
        return Error::from_syscall(rv);
    return rv;
}

ErrorOr<usize> write(int fd, StringView string)
{
    return TRY(write(fd, string.data(), string.size()));
}

ErrorOr<usize> write(int fd, MappedFile const& file)
{
    return TRY(write(fd, file.data(), file.size()));
}

ErrorOr<usize> write(int fd, StringBuffer const& string)
{
    return TRY(write(fd, string.data(), string.size()));
}

ErrorOr<usize> writev(int fd, IOVec const* iovec, int count)
{
    auto rv = syscall(Syscall::writev, fd, iovec, count);
    if (rv < 0)
        return Error::from_syscall(rv);
    return rv;
}

ErrorOr<u8*> mmap(void* addr, usize size, int prot, int flags,
    int fd, long offset)
{
    auto rv = syscall(Syscall::mmap, addr, size, prot, flags, fd,
        offset);
    if (rv < 0)
        return Error::from_syscall(rv);
    return (u8*)rv;
}

ErrorOr<u8*> mmap(usize size, int prot, int flags, int fd,
    long offset)
{
    return TRY(mmap(nullptr, size, prot, flags, fd, offset));
}

ErrorOr<void> munmap(void const* addr, usize size)
{
    auto rv = syscall(Syscall::munmap, addr, size);
    if (rv < 0)
        return Error::from_syscall(rv);
    return {};
}

ErrorOr<void> mprotect(void* addr, usize len, int prot)
{
    auto rv = syscall(Syscall::mprotect, addr, len, prot);
    if (rv < 0)
        return Error::from_syscall(rv);
    return {};
}

ErrorOr<void> remove(c_string path)
{
    // Assume not directory.
    auto rv = syscall(Syscall::unlinkat, AT_FDCWD, path, 0);
    if (rv == -EISDIR) {
        // Oops, was directory.
        rv = syscall(Syscall::unlinkat, AT_FDCWD, path,
            AT_REMOVEDIR);
    }
    if (rv < 0)
        return Error::from_syscall(rv);
    return {};
}

ErrorOr<int> open(c_string path, int flags)
{
    if ((flags & O_CREAT) != 0) {
        return Error::from_string_literal(
            "O_CREAT should not be used with this function "
            "variant");
    }
    auto fd = syscall(Syscall::open, path, flags, 0);
    if (fd < 0)
        return Error::from_syscall(fd);
    return (int)fd;
}

ErrorOr<int> open(c_string path, int flags, mode_t mode)
{
    auto fd = syscall(Syscall::open, path, flags | O_CREAT, mode);
    if (fd < 0)
        return Error::from_syscall(fd);
    return (int)fd;
}

ErrorOr<int> open(StringView path, int flags)
{
    auto buf = TRY(StringBuffer::create_fill(path, "\0"sv));
    return TRY(open(buf.data(), flags));
}

ErrorOr<int> open(StringView path, int flags, mode_t mode)
{
    auto buf = TRY(StringBuffer::create_fill(path, "\0"sv));
    return TRY(open(buf.data(), flags, mode));
}

ErrorOr<int> openat(int root_fd, StringView path, int flags)
{
    if ((flags & O_CREAT) != 0) {
        return Error::from_string_literal(
            "O_CREAT should not be used with this function "
            "variant");
    }
    auto buf = TRY(StringBuffer::create_fill(path, "\0"sv));
    auto fd = ::openat(root_fd, buf.data(), flags);
    if (fd < 0)
        return Error::from_errno();
    return fd;
}

ErrorOr<int> openat(int root_fd, StringView path, int flags, mode_t mode)
{
    auto buf = TRY(StringBuffer::create_fill(path, "\0"sv));
    auto fd = ::openat(root_fd, buf.data(), flags | O_CREAT, mode);
    if (fd < 0)
        return Error::from_errno();
    return fd;
}

ErrorOr<void> close(int fd)
{
    TRY(fsync(fd));
    auto rv = syscall(Syscall::close, fd);
    if (rv < 0)
        return Error::from_syscall(rv);
    return {};
}

ErrorOr<isize> read(int fd, void* buf, usize bufsize)
{
    auto result = ::read(fd, buf, bufsize);
    if (result < 0) {
        return Error::from_errno();
    }
    return result;
}

ErrorOr<void> ftruncate(int fd, usize length)
{
    if (::ftruncate(fd, (off_t)length) < 0) {
        return Error::from_errno();
    }
    return {};
}

ErrorOr<void> unlink(c_string path)
{
    auto rv = syscall(Syscall::unlink, path);
    if (rv < 0)
        return Error::from_syscall(rv);
    return {};
}

ErrorOr<pid_t> posix_spawnp(c_string file, c_string const* argv,
    c_string const* envp,
    posix_spawn_file_actions_t const* file_actions,
    posix_spawnattr_t const* attrp)
{
    pid_t pid = -1;
    auto rc = ::posix_spawnp(&pid, file, file_actions, attrp,
        (char**)argv, (char**)envp);
    if (rc != 0)
        return Error::from_errno(rc);
    if (pid < 0)
        return Error::from_errno();
    return pid;
}

ErrorOr<Status> waitpid(pid_t pid, int options)
{
    int status = 0;
    if (::waitpid(pid, &status, options) < 0)
        return Error::from_errno();
    return Status { .raw = status };
}

#ifdef __linux__
#    define TIOCGETD 0x5424
#elif __APPLE__
#    define TIOCGETD 0x40047426
#else
#    warning "unimplemented"
#endif

bool isatty(int fd)
{
    int line_discipline = 0x1234abcd;
    // This gets the line discipline of the terminal. When called on
    // something that isn't a terminal it doesn't change
    // `line_discipline` and returns -1.
    auto rv
        = syscall(Syscall::ioctl, fd, TIOCGETD, &line_discipline);
    if (rv == ENOTTY)
        return false;
    return rv == 0;
}

ErrorOr<long> sysconf(int name)
{
    auto value = ::sysconf(name);
    if (value == -1)
        return Error::from_errno();
    return value;
}

ErrorOr<u32> page_size()
{
    static u32 size = 0;
    if (size == 0) [[unlikely]] {
        size = (u32)TRY(Core::System::sysconf(_SC_PAGESIZE));
    }
    return size;
}

Optional<c_string> getenv(StringView name)
{
    for (u32 i = 0; environ[i] != nullptr; i++) {
        auto env = StringView::from_c_string(environ[i]);
        if (env.starts_with(name)) {
            auto maybe_value_index = env.find_first('=');
            if (!maybe_value_index.has_value())
                return "";
            auto value_index = maybe_value_index.value();
            return &env[value_index + 1];
        }
    }
    return {};
}

ErrorOr<bool> has_program(StringView name)
{
    auto maybe_path = getenv("PATH"sv);
    if (!maybe_path.has_value())
        return Error::from_string_literal("PATH not found");
    auto path = StringView::from_c_string(maybe_path.value());

    auto file_path = StringBuffer();
    auto paths = TRY(path.split_on(':'));
    for (auto directory : paths) {
        Defer clear_file_path = [&] {
            file_path.clear();
        };

        TRY(file_path.write(directory, "/"sv, name, "\0"sv));

        auto stat_result = stat(file_path.data());
        if (stat_result.is_error())
            continue;
        auto file = stat_result.release_value();
        return file.is_executable();
    }

    return false;
}

[[noreturn]] void exit(int code)
{
    syscall(Syscall::exit, code);
    __builtin_unreachable();
}

ErrorOr<void> mkdir(StringView path, mode_t mode)
{
    auto buf = TRY(StringBuffer::create_fill(path, "\0"sv));
    auto result = ::mkdir(buf.data(), mode);
    if (result < 0) {
        return Error::from_errno(errno);
    }
    return {};
}

ErrorOr<void> mkdirat(int fd, StringView path, mode_t mode)
{
    auto buf = TRY(StringBuffer::create_fill(path, "\0"sv));
    auto result = ::mkdirat(fd, buf.data(), mode);
    if (result < 0) {
        return Error::from_errno(errno);
    }
    return {};
}

}
