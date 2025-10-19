#pragma once
#include "./Base.h"

#include "./MemoryPoker.h"
#include "./TypeId.h"
#include "./Bits.h"
#include "./Error.h"

#include <pthread.h>

#define DEFINE_MESSAGE(T) \
    typedef struct T const* T ## Ref; TYPE_REGISTER(T ##Ref); \
    typedef struct T T; TYPE_REGISTER(T); struct T


typedef struct [[nodiscard]] {
    bool found;
} MailboxStatus;
C_INLINE MailboxStatus mailbox_empty(void) { return (MailboxStatus){false}; }
C_INLINE MailboxStatus mailbox_found(void) { return (MailboxStatus){true}; }


constexpr u64 message_size_max = 4294967295;
constexpr u64 message_align_max = 65535;
constexpr u64 mailbox_port_max = 32;

typedef struct Mailbox Mailbox;
typedef struct MailboxReader MailboxReader;
typedef struct MailboxWriter MailboxWriter;

typedef struct { bool did_timeout; } MailboxDidTimeout;

struct timespec;
typedef struct Mailbox {
    u64 version;

    u8* items;
    _Atomic u64 read_offset;
    _Atomic u64 write_offset;
    u64 capacity;

    struct {
        pthread_t thread;
        bool is_tied;
    } reader_thread;
    struct {
        pthread_t thread;
        bool is_tied;
    } writer_thread;

#ifdef __cplusplus
    MailboxReader* reader(pthread_t = pthread_self());
    MailboxWriter* writer(pthread_t = pthread_self());

    void attach_memory_poker(MemoryPoker*) const;
#endif
} Mailbox;

typedef struct MailboxReader {
    Mailbox mailbox;

#ifdef __cplusplus
    MailboxStatus peek(u16* tag) const;

    template <typename T>
    MailboxStatus peek(T* buf) const { return peek(Ty2::type_id<T>(), buf, sizeof(T), alignof(T)); }

    MailboxStatus peek(u16 tag, void*, u64 size, u64 align) const;

    KError read(u16 tag, u64 size, u64 align, void*);

    template <typename T>
    KError read(T* out) { return read(Ty2::type_id<T>(), sizeof(T), alignof(T), out); }

    void toss(u16 tag);

    MailboxDidTimeout wait(struct timespec const* timeout = nullptr);
    MailboxDidTimeout wait(struct timespec const& timeout) { return wait(&timeout); }
#endif
} MailboxReader;
static_assert(sizeof(MailboxReader) == sizeof(Mailbox));
static_assert(OFFSET_OF(MailboxReader, mailbox) == 0);

typedef struct MailboxWriter {
    Mailbox mailbox;

#ifdef __cplusplus
    template <typename T>
        requires (sizeof(T) <= message_size_max) && (alignof(T) <= message_align_max)
    KError post(T const& value) { return post(Ty2::type_id<T>(), sizeof(T), alignof(T), &value); }
    KError post(u16 tag, u64 size, u64 align, void const* data);
#endif
} MailboxWriter;
static_assert(sizeof(MailboxWriter) == sizeof(Mailbox));
static_assert(OFFSET_OF(MailboxWriter, mailbox) == 0);

typedef struct MailboxPort { u64 id; } MailboxPort;

typedef struct MailboxReaderSet MailboxReaderSet;
typedef struct MailboxPorts {
    Mailbox mailbox[mailbox_port_max][mailbox_port_max]; // receiver port, sender
    pthread_t sender[mailbox_port_max][mailbox_port_max]; // receiver port, sender
    u8 sender_count[mailbox_port_max]; // receiver port
    u8 port_is_important[mailbox_port_max / 8];
    _Atomic u64 allocated_ports;
    MemoryPoker* poker;

#ifdef __cplusplus
    void attach_memory_poker(MemoryPoker*);

    KError reserve_port(MailboxPort* out, u64 min_capacity = 1 * MiB);
    KError reserve_important_port(MailboxPort* out, u64 min_capacity = 1 * MiB);

    MailboxReaderSet* reader(MailboxPort reader, pthread_t = pthread_self());
    MailboxWriter* writer(MailboxPort receiver, pthread_t = pthread_self());
#endif
} MailboxPorts;
static_assert(sizeof(MailboxPorts) < 96 * KiB);

C_API KError mailbox_init(u32 min_capacity, Mailbox*);
C_API KError mailbox_lazy_init(u32 min_capacity, Mailbox*);
C_API MailboxReader* mailbox_reader(Mailbox*, pthread_t);
C_API MailboxWriter* mailbox_writer(Mailbox*, pthread_t);
#ifndef __cplusplus
#define mailbox_writer(mailbox) mailbox_writer((mailbox), pthread_self())
#define mailbox_reader(mailbox) mailbox_writer((mailbox), pthread_self())
#endif

C_API void mailbox_attach_memory_poker(Mailbox const*, MemoryPoker*);

C_API KError mailbox_post(MailboxWriter*, u16 tag, u64 size, u64 align, void const* data);
#ifndef __cplusplus
#define mailbox_post(writer, tag, data) mailbox_post(writer, tag, sizeof(*data), alignof(__typeof(*data)), data)
#endif

C_API KError mailbox_read(MailboxReader*, u16 tag, u64 size, u64 align, void*);
C_API MailboxStatus mailbox_peek(MailboxReader const*, u16* tag);
C_API MailboxStatus mailbox_peek2(MailboxReader const*, u16 tag, void*, u64 size, u64 align);
C_API void mailbox_toss(MailboxReader*, u16 tag);
C_API MailboxDidTimeout mailbox_wait(MailboxReader*, struct timespec const* timeout);

C_API MailboxDidTimeout mailbox_wait_any(struct timespec const* timeout);
#ifdef __cplusplus
static inline MailboxDidTimeout mailbox_wait_any(struct timespec const& timeout) { return mailbox_wait_any(&timeout); }
static inline MailboxDidTimeout mailbox_wait_any() { return mailbox_wait_any(nullptr); }
#endif

C_API void mailbox_ports_attach_memory_poker(MailboxPorts*, MemoryPoker*);
C_API KError mailbox_ports_reserve_port(MailboxPorts*, MailboxPort* out, u64 min_capacity);
C_API KError mailbox_ports_reserve_important_port(MailboxPorts*, MailboxPort* out, u64 min_capacity);
C_API MailboxReaderSet* mailbox_ports_reader(MailboxPorts*, MailboxPort reader, pthread_t);
C_API MailboxWriter* mailbox_ports_writer(MailboxPorts*, MailboxPort receiver, pthread_t);
