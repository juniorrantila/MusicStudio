#include "System.h"
#include "Defer.h"
#include "Formatter.h"
#include "StringBuffer.h"
#include <fcntl.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#if __APPLE__
#include <signal.h>
#endif

namespace Ty::System {

ErrorOr<void> fsync(int fd)
{
    auto rv = ::fsync(fd);
    if (rv < 0)
        return Error::from_errno();
    return {};
}

ErrorOr<Stat> fstat(int fd)
{
    struct stat buf;
    auto rv = ::fstat(fd, &buf);
    if (rv < 0) {
        return Error::from_errno();
    }
    return Stat(buf);
}

ErrorOr<Stat> stat(c_string path)
{
    struct stat buf;
    auto rv = ::stat(path, &buf);
    if (rv < 0) {
        return Error::from_errno();
    }
    return Stat(buf);
}

ErrorOr<usize> write(int fd, void const* data, usize size)
{
    auto rv = ::write(fd, data, size);
    if (rv < 0) {
        return Error::from_errno();
    }
    return (usize)rv;
}

ErrorOr<usize> write(int fd, StringView string)
{
    return TRY(write(fd, string.data(), string.size()));
}

ErrorOr<usize> write(int fd, StringBuffer const& string)
{
    return TRY(write(fd, string.data(), string.size()));
}

ErrorOr<usize> writev(int fd, IOVec const* iovec, int count)
{
    auto rv = ::writev(fd, (struct iovec*)iovec, count);
    if (rv < 0)
        return Error::from_syscall(rv);
    return rv;
}

ErrorOr<u8*> mmap(void* addr, usize size, int prot, int flags,
    int fd, long offset)
{
    auto* rv = ::mmap(addr, size, prot, flags, fd, offset);
    if (rv == (void*)-1)
        return Error::from_errno();
    return (u8*)rv;
}

ErrorOr<u8*> mmap(usize size, int prot, int flags, int fd,
    long offset)
{
    return TRY(mmap(nullptr, size, prot, flags, fd, offset));
}

ErrorOr<void> munmap(void const* addr, usize size)
{
    auto rv = ::munmap((void*)addr, size);
    if (rv < 0)
        return Error::from_errno();
    return {};
}

ErrorOr<void> mprotect(void* addr, usize len, int prot)
{
    auto rv = ::mprotect(addr, len, prot);
    if (rv < 0)
        return Error::from_errno();
    return {};
}

ErrorOr<void> remove(c_string path)
{
    // Assume not directory.
    auto rv = ::unlinkat(AT_FDCWD, path, 0);
    if (rv == -EISDIR) {
        // Oops, was directory.
        rv = ::unlinkat(AT_FDCWD, path, AT_REMOVEDIR);
    }
    if (rv < 0)
        return Error::from_errno();
    return {};
}

ErrorOr<int> open(c_string path, int flags)
{
    if ((flags & O_CREAT) != 0) {
        return Error::from_string_literal(
            "O_CREAT should not be used with this function "
            "variant");
    }
    auto fd = ::open(path, flags, 0);
    if (fd < 0)
        return Error::from_errno();
    return (int)fd;
}

ErrorOr<int> open(c_string path, int flags, mode_t mode)
{
    auto fd = ::open(path, flags | O_CREAT, mode);
    if (fd < 0)
        return Error::from_errno();
    return (int)fd;
}

ErrorOr<void> close(int fd)
{
    auto rv = ::close(fd);
    if (rv < 0)
        return Error::from_errno();
    return {};
}

ErrorOr<void> unlink(c_string path)
{
    auto rv = ::unlink(path);
    if (rv < 0)
        return Error::from_errno();
    return {};
}

ErrorOr<int> mkstemps(char* template_, int suffixlen)
{
    int fd = ::mkstemps(template_, suffixlen);
    if (fd < 0)
        return Error::from_errno();
    return fd;
}

ErrorOr<int> mkstemps(char* template_)
{
    return TRY(mkstemps(template_, 0));
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
#else
#    warning "unimplemented"
#endif

bool isatty(int fd)
{
    int line_discipline = 0x1234abcd;
    // This gets the line discipline of the terminal. When called on
    // something that isn't a terminal it doesn't change
    // `line_discipline` and returns -1.
    auto rv = ioctl(fd, TIOCGETD, &line_discipline);
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
        size = (u32)TRY(System::sysconf(_SC_PAGESIZE));
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

void sleep(u32 seconds) { ::sleep(seconds); }

[[noreturn]] void exit(int code)
{
    ::exit(code);
    __builtin_unreachable();
}

ErrorOr<int> fork()
{
    auto pid = ::fork();
    if (pid < 0)
        return Error::from_errno();
    return pid;
}

#if __APPLE__

#pragma push_macro("sigemptyset")
#undef sigemptyset
ErrorOr<void> sigemptyset(sigset_t* set)
{
    *set = { 0 };
    return {};
}
#pragma pop_macro("sigemptyset")

#else

ErrorOr<void> sigemptyset(sigset_t* set)
{
    auto rv = ::sigemptyset(set);
    if (rv < 0)
        return Error::from_errno();
    return {};
}

#endif

ErrorOr<void> sigaction(int sig,
    const struct sigaction* __restrict action,
    struct sigaction* __restrict old_action)
{
    auto rv = ::sigaction(sig, action, old_action);
    if (rv < 0)
        return Error::from_errno();
    return {};
}

ErrorOr<int> socket(int domain, int type, int protocol)
{
    auto rv = ::socket(domain, type, protocol);
    if (rv < 0)
        return Error::from_errno();
    return rv;
}

ErrorOr<void> bind(int fd, struct sockaddr const* addr,
    socklen_t len)
{
    if (::bind(fd, addr, len) < 0)
        return Error::from_errno();
    return {};
}

ErrorOr<void> listen(int fd, int number_of_clients)
{
    if (::listen(fd, number_of_clients) < 0)
        return Error::from_errno();
    return {};
}

ErrorOr<void> connect(int fd, struct sockaddr const* addr,
    socklen_t len)
{
    if (::connect(fd, addr, len) < 0)
        return Error::from_errno();
    return {};
}

ErrorOr<ssize_t> recv(int fd, void* buf, size_t buf_size, int flags)
{
    auto rv = ::recv(fd, buf, buf_size, flags);
    if (rv < 0)
        return Error::from_errno();
    return rv;
}

ErrorOr<ssize_t> send(int fd, void const* buf, size_t buf_size,
    int flags)
{
    auto rv = ::send(fd, buf, buf_size, flags);
    if (rv < 0)
        return Error::from_errno();
    return rv;
}

ErrorOr<ssize_t> send(int fd, StringView view, int flags)
{
    return TRY(send(fd, view.data(), view.size(), flags));
}

ErrorOr<struct addrinfo*> getaddrinfo(u16 service,
    struct addrinfo hints)
{
    struct addrinfo* res;

    auto service_buf
        = TRY(StringBuffer::create_fill(service, "\0"sv));
    auto rv
        = getaddrinfo(nullptr, service_buf.data(), &hints, &res);
    if (rv < 0)
        return Error::from_errno();

    return res;
}

ErrorOr<struct addrinfo*> getaddrinfo(StringView name, u16 service,
    struct addrinfo hints)
{
    struct addrinfo* res;

    auto name_buf = TRY(StringBuffer::create_fill(name, "\0"sv));
    auto service_buf
        = TRY(StringBuffer::create_fill(service, "\0"sv));

    auto rv = getaddrinfo(name_buf.data(), service_buf.data(),
        &hints, &res);
    if (rv < 0)
        return Error::from_errno();

    return res;
}

ErrorOr<struct addrinfo*> getaddrinfo(StringView name,
    StringView service, struct addrinfo hints)
{
    struct addrinfo* res;

    auto name_buf = TRY(StringBuffer::create_fill(name, "\0"sv));
    auto service_buf
        = TRY(StringBuffer::create_fill(service, "\0"sv));

    auto rv = getaddrinfo(name_buf.data(), service_buf.data(),
        &hints, &res);
    if (rv < 0)
        return Error::from_errno();

    return res;
}

ErrorOr<void> setsockopt(int fd, int level, int optname, int value)
{
    auto rv
        = ::setsockopt(fd, level, optname, &value, sizeof(value));
    if (rv < 0)
        return Error::from_errno();
    return {};
}

static void* get_in_addr(struct sockaddr* sa)
{
    if (sa->sa_family == AF_INET)
        return &((struct sockaddr_in*)sa)->sin_addr;
    return &((struct sockaddr_in6*)sa)->sin6_addr;
}

ErrorOr<StringBuffer> inet_ntop(struct sockaddr_storage sa)
{
    char buf[INET6_ADDRSTRLEN + 1];
    c_string res = ::inet_ntop(sa.ss_family,
        get_in_addr((struct sockaddr*)&sa), buf, sizeof(buf));
    if (res == nullptr)
        return Error::from_errno();
    return StringBuffer::create_fill(
        StringView::from_c_string(buf));
}

}

int meaning_of_life(void) { return 42; }
