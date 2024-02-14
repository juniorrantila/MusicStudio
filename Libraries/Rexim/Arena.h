// Copyright 2022 Alexey Kutepov <reximkut@gmail.com>

// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:

// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once
#include <Ty/Base.h>

struct Region {
    Region *next;
    usize count;
    usize capacity;
    uptr data[];
};

struct Arena {
    Region *begin, *end;
};

#define REGION_DEFAULT_CAPACITY (8*1024)

Region *new_region(usize capacity);
void free_region(Region *r);

// TODO: snapshot/rewind capability for the arena
// - Snapshot should be combination of a->end and a->end->count.
// - Rewinding should be restoring a->end and a->end->count from the snapshot and
// setting count-s of all the Region-s after the remembered a->end to 0.
void *arena_alloc(Arena *a, usize size_bytes);
void *arena_realloc(Arena *a, void *oldptr, usize oldsz, usize newsz);

void arena_reset(Arena *a);
void arena_free(Arena *a);
