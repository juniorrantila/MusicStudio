#pragma once
#include "./Base.h"

#include "./MemoryPoker.h"

#ifdef __cplusplus
#include "./TypeId.h"
#endif

#include <pthread.h>

typedef struct [[nodiscard]] { bool ok; } MailboxSuccess;
C_INLINE MailboxSuccess mailbox_ok(void) { return (MailboxSuccess){true}; }
C_INLINE MailboxSuccess mailbox_error(void) { return (MailboxSuccess){false}; }
C_INLINE MailboxSuccess mailbox_bad_argument(void) { return (MailboxSuccess){false}; }
C_INLINE MailboxSuccess mailbox_no_space(void) { return (MailboxSuccess){false}; }


typedef struct [[nodiscard]] {
    bool found;
} MailboxStatus;
C_INLINE MailboxStatus mailbox_empty(void) { return (MailboxStatus){false}; }
C_INLINE MailboxStatus mailbox_found(void) { return (MailboxStatus){true}; }


constexpr u64 message_size_max = 1274;
constexpr u64 message_align_max = 65535;
typedef struct alignas(256) Message {
    u16 tag;
    u16 align;
    u16 size;
    u8 data[message_size_max];

#ifdef __cplusplus
    MailboxSuccess unwrap(u16 tag, u64 size, u64 align, void*) const;

    template <typename T>
    MailboxSuccess unwrap(T* buf) const { return unwrap(Ty2::type_id<T>(), sizeof(T), alignof(T), buf); }
#endif
} Message;
static_assert(sizeof(Message) == 1280);

C_API MailboxSuccess message_unwrap(Message const*, u16 tag, u64 size, u64 align, void*);

typedef struct Mailbox Mailbox;
typedef struct MailboxReader MailboxReader;
typedef struct MailboxWriter MailboxWriter;

typedef struct { bool did_timeout; } MailboxDidTimeout;

struct timespec;
typedef struct Mailbox {
    Message* items;
    _Atomic u32 read_offset;
    _Atomic u32 write_offset;
    u32 capacity;

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
    MailboxStatus peek(Message*) const;
    void toss(Message const*);
    MailboxStatus read(Message*);

    MailboxDidTimeout wait(struct timespec const* timeout = nullptr);
    MailboxDidTimeout wait(struct timespec const& timeout) { return wait(&timeout); }
#endif
} MailboxReader;
static_assert(sizeof(MailboxReader) == sizeof(Mailbox));
static_assert(ty_offsetof(MailboxReader, mailbox) == 0);

typedef struct MailboxWriter {
    Mailbox mailbox;

#ifdef __cplusplus
    template <typename T>
        requires (sizeof(T) <= message_size_max) && (alignof(T) <= message_align_max)
    MailboxSuccess post(T const& value) { return post(Ty2::type_id<T>(), sizeof(T), alignof(T), &value); }
    MailboxSuccess post(u16 tag, u64 size, u64 align, void const* data);
#endif
} MailboxWriter;
static_assert(sizeof(MailboxWriter) == sizeof(Mailbox));
static_assert(ty_offsetof(MailboxWriter, mailbox) == 0);

C_API MailboxSuccess mailbox_init(u32 min_items, Mailbox*);
C_API MailboxReader* mailbox_reader(Mailbox*, pthread_t);
C_API MailboxWriter* mailbox_writer(Mailbox*, pthread_t);

C_API void mailbox_attach_memory_poker(Mailbox const*, MemoryPoker*);

C_API MailboxSuccess mailbox_post(MailboxWriter*, u16 tag, u64 size, u64 align, void const* data);
C_API MailboxStatus mailbox_read(MailboxReader*, Message*);
C_API MailboxStatus mailbox_peek(MailboxReader const*, Message*);
C_API void mailbox_toss(MailboxReader*, Message const*);
C_API MailboxDidTimeout mailbox_wait(MailboxReader*, struct timespec const* timeout);

C_API MailboxDidTimeout mailbox_wait_any(struct timespec const* timeout);
#ifdef __cplusplus
static inline MailboxDidTimeout mailbox_wait_any(struct timespec const& timeout) { return mailbox_wait_any(&timeout); }
static inline MailboxDidTimeout mailbox_wait_any() { return mailbox_wait_any(nullptr); }
#endif
