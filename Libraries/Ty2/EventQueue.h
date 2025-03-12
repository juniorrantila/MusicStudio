#pragma once
#include "./Base.h"

#include "./Allocator.h"

typedef struct Event {
    usize kind;
    usize payload_size;
    alignas(16) u8 payload[];
} Event;
static_assert(sizeof(Event) == 2 * sizeof(void*));

typedef struct EventSegment {
    struct EventSegment* next;
    usize count;
    struct Event* items[14];
} EventSegment;
static_assert(sizeof(EventSegment) == 16 * sizeof(void*));

typedef struct EventQueue {
    Allocator* arena;
} EventQueue;

EventQueue event_queue_create(void);
void event_queue_destroy(EventQueue*);

void event_queue_drain(EventQueue*);

bool event_emit(EventQueue*, usize kind, void const* payload, usize payload_size);
Event* event_poll(EventQueue*);
