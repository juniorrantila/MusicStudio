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


constexpr u64 message_size_max = 4294967295;
constexpr u64 message_align_max = 65535;

typedef struct Mailbox Mailbox;
typedef struct MailboxReader MailboxReader;
typedef struct MailboxWriter MailboxWriter;

typedef struct { bool did_timeout; } MailboxDidTimeout;

struct timespec;
typedef struct Mailbox {
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
    MailboxSuccess read(u16 tag, u64 size, u64 align, void*);

    template <typename T>
    MailboxSuccess read(T* out) { return read(Ty2::type_id<T>(), sizeof(T), alignof(T), out); }

    void toss(u16 tag);

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

C_API MailboxSuccess mailbox_init(u32 min_capacity, Mailbox*);
C_API MailboxReader* mailbox_reader(Mailbox*, pthread_t);
C_API MailboxWriter* mailbox_writer(Mailbox*, pthread_t);

C_API void mailbox_attach_memory_poker(Mailbox const*, MemoryPoker*);

C_API MailboxSuccess mailbox_post(MailboxWriter*, u16 tag, u64 size, u64 align, void const* data);
C_API MailboxSuccess mailbox_read(MailboxReader*, u16 tag, u64 size, u64 align, void*);
C_API MailboxStatus mailbox_peek(MailboxReader const*, u16* tag);
C_API void mailbox_toss(MailboxReader*, u16 tag);
C_API MailboxDidTimeout mailbox_wait(MailboxReader*, struct timespec const* timeout);

C_API MailboxDidTimeout mailbox_wait_any(struct timespec const* timeout);
#ifdef __cplusplus
static inline MailboxDidTimeout mailbox_wait_any(struct timespec const& timeout) { return mailbox_wait_any(&timeout); }
static inline MailboxDidTimeout mailbox_wait_any() { return mailbox_wait_any(nullptr); }
#endif
