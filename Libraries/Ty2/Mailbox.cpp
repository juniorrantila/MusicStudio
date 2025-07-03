#include "./Mailbox.h"

#include "./Verify.h"
#include "./PageAllocator.h"
#include "./Defer.h"

#include <string.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>


static Message const* read_ptr(Mailbox const*);
static Message* write_ptr(Mailbox const*);
static u64 fill_count(Mailbox const*);
static u64 slots_left(Mailbox const*);

static void increment_read_ptr(Mailbox*);
static void increment_write_ptr(Mailbox*);

MailboxSuccess Message::unwrap(u16 tag, u64 size, u64 align, void* buf) const { return message_unwrap(this, tag, size, align, buf); }
MailboxSuccess message_unwrap(Message const* message, u16 tag, u64 size, u64 align, void* buf)
{
    if (verify(message->tag == tag).failed) return mailbox_bad_argument(); 
    if (verify(message->size == size).failed) return mailbox_bad_argument(); 
    if (verify(message->align == align).failed) return mailbox_bad_argument(); 
    memcpy(buf, message->data, size);
    return mailbox_ok();
}

MailboxSuccess MailboxWriter::post(u16 tag, u64 size, u64 align, void const* data) { return mailbox_post(this, tag, size, align, data); }
C_API MailboxSuccess mailbox_post(MailboxWriter* mb, u16 tag, u64 size, u64 align, void const* data)
{
    if (verify(size <= message_size_max).failed) return mailbox_bad_argument();
    if (verify(align <= message_align_max).failed) return mailbox_bad_argument();
    VERIFY(mb->mailbox.writer_thread.is_tied);
    VERIFY(pthread_equal(mb->mailbox.writer_thread.thread, pthread_self()));
    if (slots_left(&mb->mailbox) == 0) return mailbox_no_space();

    Message* m = write_ptr(&mb->mailbox);
    *m = (Message){
        .tag = tag,
        .align = (u16)align,
        .size = (u16)size,
        .data = {},
    };
    memcpy(m->data, data, size);
    increment_write_ptr(&mb->mailbox);

    if (mb->mailbox.reader_thread.is_tied) {
        pthread_kill(mb->mailbox.reader_thread.thread, SIGCONT);
    }
    return mailbox_ok();
}

MailboxStatus MailboxReader::read(Message* message) { return mailbox_read(this, message); }
C_API MailboxStatus mailbox_read(MailboxReader* mb, Message* out)
{
    VERIFY(mb->mailbox.reader_thread.is_tied);
    VERIFY(pthread_equal(mb->mailbox.reader_thread.thread, pthread_self()));
    if (mb->peek(out).found) {
        increment_read_ptr(&mb->mailbox);
        return mailbox_found();
    }
    return mailbox_empty();
}

MailboxStatus MailboxReader::peek(Message* message) const { return mailbox_peek(this, message); }
C_API MailboxStatus mailbox_peek(MailboxReader const* mb, Message* out)
{
    VERIFY(mb->mailbox.reader_thread.is_tied);
    VERIFY(pthread_equal(mb->mailbox.reader_thread.thread, pthread_self()));
    if (fill_count(&mb->mailbox) == 0) return mailbox_empty();
    if (!out) return mailbox_found();
    memcpy(out, read_ptr(&mb->mailbox), sizeof(Message));
    return mailbox_found();
}

void MailboxReader::toss(Message const* message) { return mailbox_toss(this, message); }
C_API void mailbox_toss(MailboxReader* mb, Message const*)
{
    VERIFY(mb->mailbox.reader_thread.is_tied);
    VERIFY(pthread_equal(mb->mailbox.reader_thread.thread, pthread_self()));
    if (verify(mailbox_peek(mb, nullptr).found).failed) return;
    increment_read_ptr(&mb->mailbox);
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
    return mb->items + (read_offset % mb->capacity);
}

static Message* write_ptr(Mailbox const* mb)
{
    VERIFY(mb->writer_thread.is_tied);
    VERIFY(pthread_equal(mb->writer_thread.thread, pthread_self()));
    u64 write_offset = mb->write_offset;
    return mb->items + (write_offset % mb->capacity);
}

static void increment_read_ptr(Mailbox* mb)
{
    VERIFY(mb->reader_thread.is_tied);
    VERIFY(pthread_equal(mb->reader_thread.thread, pthread_self()));
    ++mb->read_offset;
    VERIFY(fill_count(mb) >= 0);
}

static void increment_write_ptr(Mailbox* mb)
{
    VERIFY(mb->writer_thread.is_tied);
    VERIFY(pthread_equal(mb->writer_thread.thread, pthread_self()));
    ++mb->write_offset;
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
    VERIFY(count <= (i64)mb->capacity);
    return count;
}

static u64 slots_left(Mailbox const* mb)
{
    return mb->capacity - fill_count(mb);
}

static inline u64 ceil_f64_to_u64(f64 x)
{
    const f64 truncation = (f64)(u64)x;
    return (u64)(truncation + (truncation < x));
}

C_API MailboxSuccess mailbox_init(u32 min_items, Mailbox* mb)
{
    if (verify(min_items != 0).failed) return mailbox_bad_argument();

    u32 min_capacity = min_items * sizeof(Message);
    u64 actual_capacity = ceil_f64_to_u64((f64)min_capacity / (f64)page_size()) * page_size();
    u32 actual_items = actual_capacity / sizeof(Message);

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

    if (ftruncate(fd, (off_t)actual_capacity))
        return mailbox_error();

    u8* address = (u8*)mmap(NULL, actual_capacity * 2, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (address == MAP_FAILED)
        return mailbox_error();
    Defer unmap_address = [&]{
        munmap(address, 2 * actual_capacity);
    };

    u8* other_address = (u8*)mmap(address, actual_capacity, PROT_READ|PROT_WRITE,
            MAP_FIXED|MAP_SHARED, fd, 0);
    if (other_address != address)
        return mailbox_error();

    other_address = (u8*)mmap(address + actual_capacity, actual_capacity,
            PROT_READ|PROT_WRITE, MAP_FIXED|MAP_SHARED, fd, 0);
    if (other_address != address + actual_capacity)
        return mailbox_error();

    unmap_address.disarm();
    *mb = (Mailbox){
        .items = (Message*)address,
        .read_offset = 0u,
        .write_offset = 0u,
        .capacity = actual_items,
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
