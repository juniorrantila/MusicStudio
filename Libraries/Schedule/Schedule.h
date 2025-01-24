#pragma once
#include <Ty/Allocator.h>
#ifdef __cplusplus
#include <Ty/Traits.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum : u16 {
    e_sched_none = 0,
    e_sched_could_not_get_hardware_concurrency,
    e_sched_could_not_create_threads,
    e_sched_out_of_memory,
} e_schedule;

typedef struct Schedule Schedule;

c_string schedule_strerror(e_schedule);
e_schedule schedule_create(Schedule**, Allocator* arena);
void schedule_destroy(Schedule*);

void schedule_chunked_for(Schedule*, usize items, void* user, void(*)(void* user, usize start, usize end, u32 thread_id));
void schedule_parallel_for(Schedule*, usize items, void* user, void(*)(void* user, usize index, u32 thread_id));

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
static inline c_string to_c_string(e_schedule error) { return schedule_strerror(error); }

struct ScheduleRef {
    Schedule* handle { nullptr };

    static ErrorOr<ScheduleRef> create(Allocator* arena)
    {
        Schedule* handle = nullptr;
        if (auto error = schedule_create(&handle, arena)) {
            return Error::from_enum(error);
        }
        return ScheduleRef {
            .handle = handle,
        };
    }

    void destroy() const
    {
        schedule_destroy(handle);
    }

    template <typename F>
    void parallel_for(usize items, F callback) const
        requires (
            IsCallableWithArguments<F, void, usize /* index */> ||
            IsCallableWithArguments<F, void, usize /* index */, usize /* thread_id */>
        )
    {
        schedule_chunked_for(handle, items, &callback, [](void* user, usize start, usize end, u32 thread_id) {
            F& callback = *(F*)user;
            for (usize i = start; i < end; i++) {
                if constexpr (IsCallableWithArguments<F, void, usize>) {
                    callback(i);
                } else {
                    callback(i, thread_id);
                }
            }
        });
    }
};
#endif
