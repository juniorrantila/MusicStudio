#pragma once
#include "./Base.h"

typedef enum {
    MemoryPressure_Normal,
    MemoryPressure_Warning,
    MemoryPressure_Critical,
} MemoryPressure;

typedef struct MemoryPressureMonitor {
    int fd;
    MemoryPressure pressure;
    bool status_did_change;
} MemoryPressureMonitor;

C_API bool memory_pressure_monitor_init(MemoryPressureMonitor*);
C_API void memory_pressure_monitor_deinit(MemoryPressureMonitor*);

struct timespec;
C_API void memory_pressure_monitor_poll(MemoryPressureMonitor*, struct timespec const*);

C_API bool memory_pressure_did_change_to_normal(MemoryPressureMonitor const*);
C_API bool memory_pressure_did_change_to_warning(MemoryPressureMonitor const*);
C_API bool memory_pressure_did_change_to_critical(MemoryPressureMonitor const*);
