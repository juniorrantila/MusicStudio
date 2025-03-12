#pragma once
#include "./Base.h"

#include "./Allocator.h"
#include "./StringView.h"

typedef struct RopeString {
    alignas(64) char items[255 - 2 * sizeof(void*)]; // 239 bytes on 64 bit platforms.
    u8 size_left;
    struct RopeString* next;
    Allocator* gpa;

#ifdef __cplusplus
    static RopeString* from_parts(Allocator*, char const* buf, usize buf_size);
    static RopeString* from_view(Allocator*, StringView);

    void destroy();

    usize segment_size() const;
    usize size() const;
    bool is_empty() const;
    usize allocation_size() const;
    usize segment_count() const;

    bool equal(RopeString const* other) const;
    bool equal(StringView) const;

    usize copy_into(char* buf, usize buf_size) const;
    RopeString* copy(Allocator*) const;

    void append(RopeString* node);
#endif
} RopeString;
static_assert(alignof(RopeString) == 64, "allocate this object in 64 byte pool and enabled some vector optimizations");
static_assert(sizeof(RopeString) == 256, "allocate cleanly as 4 * 64 byte segments");

C_API RopeString* rs_from_parts(Allocator*, char const* buf, usize buf_size);
C_API RopeString* rs_from_view(Allocator*, StringView);
C_API void rs_destroy(RopeString*);

C_API usize rs_segment_size(RopeString const*);
C_API usize rs_size(RopeString const*);
C_API bool rs_is_empty(RopeString const*);
C_API usize rs_allocation_size(RopeString const*);
C_API usize rs_segment_count(RopeString const*);

C_API bool rs_equal(RopeString const*, RopeString const*);
C_API bool rs_equal_sv(RopeString const*, StringView);

C_API usize rs_copy_into(RopeString const*, char* buf, usize buf_size);
C_API RopeString* rs_copy(RopeString const*, Allocator*);

C_API void rs_insert(RopeString*, RopeString* node);
C_API void rs_append(RopeString*, RopeString* node);
