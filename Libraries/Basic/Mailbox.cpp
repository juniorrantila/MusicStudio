#include "./Mailbox.h"

#include "./Verify.h"
#include "./PageAllocator.h"
#include "./Defer.h"
#include "./TypeId.h"
#include "./Bits.h"
#include "./BitSet.h"
#include "./Error.h"

#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>

using nullptr_t = decltype(nullptr);
TYPE_REGISTER(nullptr_t);

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

static KError create_ring_buffer(u64 capacity, u8** out);

static KError init_if_needed(Mailbox*);

KError MailboxWriter::post(u16 tag, u64 size, u64 align, void const* data) { return mailbox_post(this, tag, size, align, data); }
C_API KError mailbox_post(MailboxWriter* mb, u16 tag, u64 size, u64 align, void const* data)
{
    guard (size <= message_size_max) else return kerror_unix(EINVAL);
    guard (align <= message_align_max) else return kerror_unix(EINVAL);
    guard (ty_type_name(tag) && "type has not been ty_type_register()'ed") else return kerror_unix(EINVAL);
    guard (mb->mailbox.writer_thread.is_tied) else return kerror_unix(EINVAL);
    guard (pthread_equal(mb->mailbox.writer_thread.thread, pthread_self())) else return kerror_unix(EINVAL);

    if (auto result = init_if_needed(&mb->mailbox); !result.ok)
        return result;
    if (bytes_left(&mb->mailbox) == 0) return kerror_unix(EWOULDBLOCK);
    if (size + sizeof(Message) >= bytes_left(&mb->mailbox))
        return kerror_unix(EWOULDBLOCK);

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
    return kerror_none;
}

KError MailboxReader::read(u16 tag, u64 size, u64 align, void* out) { return mailbox_read(this, tag, size, align, out); }
C_API KError mailbox_read(MailboxReader* mb, u16 tag, u64 size, u64 align, void* out)
{
    VERIFY(mb->mailbox.reader_thread.is_tied);
    VERIFY(pthread_equal(mb->mailbox.reader_thread.thread, pthread_self()));
    u16 tag_in_box = 0;
    if (!mb->peek(&tag_in_box).found)
        return kerror_unix(ENOENT);
    if (verify(tag_in_box == tag).failed)
        return kerror_unix(EINVAL);
    Message const* message = read_ptr(&mb->mailbox);
    if (verify(message->size == size).failed)
        return kerror_unix(EINVAL);
    if (verify(message->align == align).failed)
        return kerror_unix(EINVAL);
    memcpy(out, message->data, size);
    increment_read_ptr(&mb->mailbox, size);
    return kerror_none;
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

C_API MailboxStatus MailboxReader::peek(u16 tag, void* out, u64 size, u64 align) const { return mailbox_peek2(this, tag, out, size, align); }
C_API MailboxStatus mailbox_peek2(MailboxReader const* mb, u16 tag, void* out, u64 size, u64 align)
{
    VERIFY(out != nullptr);
    VERIFY(mb->mailbox.reader_thread.is_tied);
    VERIFY(pthread_equal(mb->mailbox.reader_thread.thread, pthread_self()));
    if (fill_count(&mb->mailbox) == 0) return mailbox_empty();
    auto const* ptr = read_ptr(&mb->mailbox);
    if (ptr->tag != tag) return mailbox_empty();
    VERIFY(align == ptr->align);
    VERIFY(size == ptr->size);
    memcpy(out, ptr->data, ptr->size);
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

C_API KError mailbox_init(u32 min_capacity, Mailbox* mb)
{
    if (verify(min_capacity != 0).failed) return kerror_unix(EINVAL);

    u64 capacity = ceil_f64_to_u64((f64)min_capacity / (f64)page_size()) * page_size();

    u8* address = nullptr;
    if (auto result = create_ring_buffer(capacity, &address); !result.ok)
        return result;

    *mb = (Mailbox){
        .version = sizeof(*mb),
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
    return kerror_none;
}

C_API KError mailbox_lazy_init(u32 min_capacity, Mailbox* mb)
{
    if (verify(min_capacity != 0).failed) return kerror_unix(EINVAL);

    u64 capacity = ceil_f64_to_u64((f64)min_capacity / (f64)page_size()) * page_size();
    *mb = (Mailbox){
        .version = sizeof(*mb),
        .items = nullptr,
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
    return kerror_none;
}

MailboxReader* Mailbox::reader(pthread_t thread) { return mailbox_reader(this, thread); }
C_API MailboxReader* mailbox_reader(Mailbox* mailbox, pthread_t thread)
{
    static_assert(sizeof(MailboxReader) == sizeof(Mailbox));
    static_assert(OFFSET_OF(MailboxReader, mailbox) == 0);
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
    static_assert(OFFSET_OF(MailboxWriter, mailbox) == 0);
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

void MailboxPorts::attach_memory_poker(MemoryPoker* poker) { return mailbox_ports_attach_memory_poker(this, poker); }
C_API void mailbox_ports_attach_memory_poker(MailboxPorts* ports, MemoryPoker* poker)
{
    ports->poker = poker;
    for (u64 i = 0; i < mailbox_port_max; i++) {
        if (bit_is_set(ports->port_is_important, i)) {
            auto* mailboxes = &ports->mailbox[i];
            for (u32 i = 0; i < ARRAY_SIZE(*mailboxes); i++) {
                mailbox_attach_memory_poker(&(*mailboxes)[i], poker);
            }
        }
    }
}

[[nodiscard]] KError MailboxPorts::reserve_port(MailboxPort* out, u64 min_capacity) { return mailbox_ports_reserve_port(this, out, min_capacity); }
C_API [[nodiscard]] KError mailbox_ports_reserve_port(MailboxPorts* ports, MailboxPort* out, u64 min_capacity)
{
    if (ports->allocated_ports >= mailbox_port_max)
        return kerror_unix(ENOMEM);
    MailboxPort port = (MailboxPort){
        .id = ports->allocated_ports,
    };
    auto* mailboxes = &ports->mailbox[port.id];
    for (u32 i = 0; i < ARRAY_SIZE(*mailboxes); i++) {
        if (auto error = mailbox_init(min_capacity, &(*mailboxes)[i]); !error.ok)
            return error;
    }

    *out = port;
    ports->allocated_ports++;
    return kerror_none;
}

[[nodiscard]] KError MailboxPorts::reserve_important_port(MailboxPort* out, u64 min_capacity) { return mailbox_ports_reserve_important_port(this, out, min_capacity); }
C_API [[nodiscard]] KError mailbox_ports_reserve_important_port(MailboxPorts* ports, MailboxPort* out, u64 min_capacity)
{
    if (auto error = mailbox_ports_reserve_port(ports, out, min_capacity); !error.ok)
        return error;
    bit_set(ports->port_is_important, out->id, true);
    if (ports->poker) {
        auto* mailboxes = &ports->mailbox[out->id];
        for (u32 i = 0; i < ARRAY_SIZE(*mailboxes); i++) {
            mailbox_attach_memory_poker(&(*mailboxes)[i], ports->poker);
        }
    }
    return kerror_none;
}

MailboxReaderSet* MailboxPorts::reader(MailboxPort reader, pthread_t self) { return mailbox_ports_reader(this, reader, self); }
C_API MailboxReaderSet* mailbox_ports_reader(MailboxPorts* ports, MailboxPort reader, pthread_t self)
{
    auto* mailboxes = &ports->mailbox[reader.id];
    for (u32 i = 0; i < ARRAY_SIZE(*mailboxes); i++) {
        (*mailboxes)[i].reader(self);
    }
    return (MailboxReaderSet*)mailboxes;
}

static bool find_writer_for_receiver(MailboxPorts const*, MailboxPort receiver, pthread_t sender, u64* index);

MailboxWriter* MailboxPorts::writer(MailboxPort receiver, pthread_t self) { return mailbox_ports_writer(this, receiver, self); }
C_API MailboxWriter* mailbox_ports_writer(MailboxPorts* ports, MailboxPort receiver, pthread_t self)
{
    auto* mailboxes = &ports->mailbox[receiver.id];
    u64 index = 0;
    if (!find_writer_for_receiver(ports, receiver, self, &index)) {
        VERIFY(ports->sender_count[receiver.id] + 1 < mailbox_port_max);
        index = ports->sender_count[receiver.id]++;
    }

    return (*mailboxes)[index].writer(self);
}

static bool find_writer_for_receiver(MailboxPorts const* ports, MailboxPort receiver, pthread_t sender, u64* index)
{
    auto const* senders = &ports->sender[receiver.id];
    for (u8 i = 0; i < ports->sender_count[receiver.id]; i++) {
        if (pthread_equal(sender, (*senders)[i])) {
            *index = i;
            return true;
        }
    }
    return false;
}

static KError init_if_needed(Mailbox* mb)
{
    VERIFY(ty_is_initialized(mb));
    if (!mb->items) {
        if (auto result = create_ring_buffer(mb->capacity, &mb->items); !result.ok) {
            return result;
        }
    }
    return kerror_none; 
}

static KError create_ring_buffer(u64 capacity, u8** out)
{
    VERIFY(capacity == ceil_f64_to_u64((f64)capacity / (f64)page_size()) * page_size());

    char shm_path[] = "/dev/shm/ty2-mailbox-XXXXXX";
    char tmp_path[] = "/tmp/ty2-mailbox-XXXXXX";

    char* chosen_path = shm_path;
    int fd = mkstemp(shm_path);
    if (fd < 0) {
        fd = mkstemp(tmp_path);
        if (fd < 0) return kerror_unix(errno);
        chosen_path = tmp_path;
    } 
    defer [&] { close(fd); };

    if (unlink(chosen_path))
        return kerror_unix(errno);

    if (ftruncate(fd, (off_t)capacity))
        return kerror_unix(errno);

    u8* address = (u8*)mmap(NULL, capacity * 2, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (address == MAP_FAILED)
        return kerror_unix(errno);
    Defer unmap_address = [&]{
        munmap(address, 2 * capacity);
    };

    u8* other_address = (u8*)mmap(address, capacity, PROT_READ|PROT_WRITE,
            MAP_FIXED|MAP_SHARED, fd, 0);
    if (other_address != address)
        return kerror_unix(errno);

    other_address = (u8*)mmap(address + capacity, capacity,
            PROT_READ|PROT_WRITE, MAP_FIXED|MAP_SHARED, fd, 0);
    if (other_address != address + capacity)
        return kerror_unix(errno);

    unmap_address.disarm();
    *out = address;
    return kerror_none;
}
