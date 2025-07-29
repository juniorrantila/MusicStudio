#include "./Mailbox.h"

#include "./Verify.h"
#include "./PageAllocator.h"
#include "./Defer.h"
#include "./TypeId.h"
#include "./Bits.h"

#include <string.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>

using nullptr_t = decltype(nullptr);
ty_type_register(nullptr_t);

typedef struct [[gnu::packed]] Message {
    u16 tag;
    u16 align; static_assert(ty_bits_fitting(message_align_max) <= 16);
    u32 size; static_assert(ty_bits_fitting(message_size_max) <= 32);
    u8 data[];
} Message;
static_assert(alignof(Message) == 1);

static Message const* read_ptr(Mailbox const*);
static Message* write_ptr(Mailbox const*);
static u64 fill_count(Mailbox const*);
static u64 bytes_left(Mailbox const*);

static void increment_read_ptr(Mailbox*, u64);
static void increment_write_ptr(Mailbox*, u64);

MailboxSuccess MailboxWriter::post(u16 tag, u64 size, u64 align, void const* data) { return mailbox_post(this, tag, size, align, data); }
C_API MailboxSuccess mailbox_post(MailboxWriter* mb, u16 tag, u64 size, u64 align, void const* data)
{
    if (verify(size <= message_size_max).failed) return mailbox_bad_argument();
    if (verify(align <= message_align_max).failed) return mailbox_bad_argument();
    VERIFYS(ty_type_name(tag) != nullptr, "type has not been ty_type_register()'ed");
    VERIFY(mb->mailbox.writer_thread.is_tied);
    VERIFY(pthread_equal(mb->mailbox.writer_thread.thread, pthread_self()));
    if (bytes_left(&mb->mailbox) == 0) return mailbox_no_space();

    if (size + sizeof(Message) >= bytes_left(&mb->mailbox))
        return mailbox_no_space();

    Message* m = write_ptr(&mb->mailbox);
    *m = (Message){
        .tag = tag,
        .align = (u16)align,
        .size = (u16)size,
    };
    memcpy(m->data, data, size);
    increment_write_ptr(&mb->mailbox, size);

    if (mb->mailbox.reader_thread.is_tied) {
        pthread_kill(mb->mailbox.reader_thread.thread, SIGCONT);
    }
    return mailbox_ok();
}

MailboxSuccess MailboxReader::read(u16 tag, u64 size, u64 align, void* out) { return mailbox_read(this, tag, size, align, out); }
C_API MailboxSuccess mailbox_read(MailboxReader* mb, u16 tag, u64 size, u64 align, void* out)
{
    VERIFY(mb->mailbox.reader_thread.is_tied);
    VERIFY(pthread_equal(mb->mailbox.reader_thread.thread, pthread_self()));
    u16 tag_in_box = 0;
    if (!mb->peek(&tag_in_box).found)
        return mailbox_error();
    if (verify(tag_in_box == tag).failed)
        return mailbox_bad_argument();
    Message const* message = read_ptr(&mb->mailbox);
    if (verify(message->size == size).failed)
        return mailbox_bad_argument();
    if (verify(message->align == align).failed)
        return mailbox_bad_argument();
    memcpy(out, message->data, size);
    increment_read_ptr(&mb->mailbox, size);
    return mailbox_ok();
}

MailboxStatus MailboxReader::peek(u16* tag) const { return mailbox_peek(this, tag); }
C_API MailboxStatus mailbox_peek(MailboxReader const* mb, u16* tag)
{
    VERIFY(tag != nullptr);
    VERIFY(mb->mailbox.reader_thread.is_tied);
    VERIFY(pthread_equal(mb->mailbox.reader_thread.thread, pthread_self()));
    if (fill_count(&mb->mailbox) == 0) return mailbox_empty();
    *tag = read_ptr(&mb->mailbox)->tag;
    return mailbox_found();
}

void MailboxReader::toss(u16 tag) { return mailbox_toss(this, tag); }
C_API void mailbox_toss(MailboxReader* mb, u16 tag)
{
    VERIFY(mb->mailbox.reader_thread.is_tied);
    VERIFY(pthread_equal(mb->mailbox.reader_thread.thread, pthread_self()));

    u16 tag_in_box = 0;
    if (!mb->peek(&tag_in_box).found)
        return;
    if (verify(tag_in_box == tag).failed)
        return;

    Message const* message = read_ptr(&mb->mailbox);
    increment_read_ptr(&mb->mailbox, message->size);
}

MailboxDidTimeout MailboxReader::wait(struct timespec const* timeout) { return mailbox_wait(this, timeout); }
C_API MailboxDidTimeout mailbox_wait(MailboxReader* mb, struct timespec const* timeout)
{
    VERIFY(mb->mailbox.reader_thread.is_tied);
    VERIFY(pthread_equal(mb->mailbox.reader_thread.thread, pthread_self()));

    if (timeout) {
        int e = nanosleep(timeout, nullptr);
        return (MailboxDidTimeout){e == 0};
    }

    sigset_t sigs;
    sigemptyset(&sigs);
    sigaddset(&sigs, SIGCONT);
    VERIFY(sigwait(&sigs, nullptr) == 0);
    return (MailboxDidTimeout){false};
}

C_API MailboxDidTimeout mailbox_wait_any(struct timespec const* timeout)
{
    if (timeout) {
        int e = nanosleep(timeout, nullptr);
        return (MailboxDidTimeout){e == 0};
    }

    sigset_t sigs;
    sigemptyset(&sigs);
    sigaddset(&sigs, SIGCONT);
    VERIFY(sigwait(&sigs, nullptr) == 0);
    return (MailboxDidTimeout){false};
}

static Message const* read_ptr(Mailbox const* mb)
{
    VERIFY(mb->reader_thread.is_tied);
    VERIFY(pthread_equal(mb->reader_thread.thread, pthread_self()));
    u64 read_offset = mb->read_offset;
    return (Message*)(mb->items + (read_offset % mb->capacity));
}

static Message* write_ptr(Mailbox const* mb)
{
    VERIFY(mb->writer_thread.is_tied);
    VERIFY(pthread_equal(mb->writer_thread.thread, pthread_self()));
    u64 write_offset = mb->write_offset;
    return (Message*)(mb->items + (write_offset % mb->capacity));
}

static void increment_read_ptr(Mailbox* mb, u64 amount)
{
    VERIFY(mb->reader_thread.is_tied);
    VERIFY(pthread_equal(mb->reader_thread.thread, pthread_self()));
    mb->read_offset += (amount + sizeof(Message));
    VERIFY(fill_count(mb) >= 0);
}

static void increment_write_ptr(Mailbox* mb, u64 amount)
{
    VERIFY(mb->writer_thread.is_tied);
    VERIFY(pthread_equal(mb->writer_thread.thread, pthread_self()));
    mb->write_offset += (amount + sizeof(Message));
    VERIFY(fill_count(mb) >= 0);
}

static u64 fill_count(Mailbox const* mb)
{
    // Whichever offset we load first might have a smaller value. So we load
    // the read_offset first.
    u64 read_offset = mb->read_offset;
    u64 write_offset = mb->write_offset;
    i64 count = (i64)write_offset - (i64)read_offset;
    VERIFY(count >= 0);
    i64 capacity = (i64)mb->capacity;
    VERIFY(capacity >= 0);
    VERIFY(count <= capacity);
    return count;
}

static u64 bytes_left(Mailbox const* mb)
{
    return mb->capacity - fill_count(mb);
}

static inline u64 ceil_f64_to_u64(f64 x)
{
    const f64 truncation = (f64)(u64)x;
    return (u64)(truncation + (truncation < x));
}

C_API MailboxSuccess mailbox_init(u32 min_capacity, Mailbox* mb)
{
    if (verify(min_capacity != 0).failed) return mailbox_bad_argument();

    u64 capacity = ceil_f64_to_u64((f64)min_capacity / (f64)page_size()) * page_size();

    char shm_path[] = "/dev/shm/ty2-mailbox-XXXXXX";
    char tmp_path[] = "/tmp/ty2-mailbox-XXXXXX";

    char* chosen_path = shm_path;
    int fd = mkstemp(shm_path);
    if (fd < 0) {
        fd = mkstemp(tmp_path);
        if (fd < 0) return mailbox_error();
        chosen_path = tmp_path;
    } 
    defer [&] { close(fd); };

    if (unlink(chosen_path))
        return mailbox_error();

    if (ftruncate(fd, (off_t)capacity))
        return mailbox_error();

    u8* address = (u8*)mmap(NULL, capacity * 2, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (address == MAP_FAILED)
        return mailbox_error();
    Defer unmap_address = [&]{
        munmap(address, 2 * capacity);
    };

    u8* other_address = (u8*)mmap(address, capacity, PROT_READ|PROT_WRITE,
            MAP_FIXED|MAP_SHARED, fd, 0);
    if (other_address != address)
        return mailbox_error();

    other_address = (u8*)mmap(address + capacity, capacity,
            PROT_READ|PROT_WRITE, MAP_FIXED|MAP_SHARED, fd, 0);
    if (other_address != address + capacity)
        return mailbox_error();

    unmap_address.disarm();
    *mb = (Mailbox){
        .items = address,
        .read_offset = 0u,
        .write_offset = 0u,
        .capacity = capacity,
        .reader_thread = {
            .thread = 0,
            .is_tied = false,
        },
        .writer_thread = {
            .thread = 0,
            .is_tied = false,
        },
    };
    return mailbox_ok();
}

MailboxReader* Mailbox::reader(pthread_t thread) { return mailbox_reader(this, thread); }
C_API MailboxReader* mailbox_reader(Mailbox* mailbox, pthread_t thread)
{
    static_assert(sizeof(MailboxReader) == sizeof(Mailbox));
    static_assert(ty_offsetof(MailboxReader, mailbox) == 0);
    if (mailbox->reader_thread.is_tied) {
        VERIFY(pthread_equal(mailbox->reader_thread.thread, thread));
    }
    mailbox->reader_thread.is_tied = true;
    mailbox->reader_thread.thread = thread;
    return (MailboxReader*)mailbox;
}

MailboxWriter* Mailbox::writer(pthread_t thread) { return mailbox_writer(this, thread); }
C_API MailboxWriter* mailbox_writer(Mailbox* mailbox, pthread_t thread)
{
    static_assert(sizeof(MailboxWriter) == sizeof(Mailbox));
    static_assert(ty_offsetof(MailboxWriter, mailbox) == 0);
    if (mailbox->writer_thread.is_tied) {
        VERIFY(pthread_equal(mailbox->writer_thread.thread, thread));
    }
    mailbox->writer_thread.thread = thread;
    mailbox->writer_thread.is_tied = true;
    return (MailboxWriter*)mailbox;
}

void Mailbox::attach_memory_poker(MemoryPoker* poker) const { return mailbox_attach_memory_poker(this, poker); }
C_API void mailbox_attach_memory_poker(Mailbox const* mailbox, MemoryPoker* poker)
{
    VERIFY(mailbox->items != nullptr);
    poker->push(mailbox->items, mailbox->capacity);
}
