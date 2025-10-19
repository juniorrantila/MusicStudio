#include "./OS.h"

#include <LibTy/Base.h>

#if __APPLE__
typedef u32 csr_config_t;

extern "C" int csr_get_active_config(csr_config_t *);
extern "C" int csr_check(csr_config_t);

#define CSR_ALLOW_UNTRUSTED_KEXTS               (1 << 0)
#define CSR_ALLOW_UNRESTRICTED_FS               (1 << 1)
#define CSR_ALLOW_TASK_FOR_PID                  (1 << 2)
#define CSR_ALLOW_KERNEL_DEBUGGER               (1 << 3)
#define CSR_ALLOW_APPLE_INTERNAL                (1 << 4)
#define CSR_ALLOW_DESTRUCTIVE_DTRACE            (1 << 5) /* name deprecated */
#define CSR_ALLOW_UNRESTRICTED_DTRACE           (1 << 5)
#define CSR_ALLOW_UNRESTRICTED_NVRAM            (1 << 6)
#define CSR_ALLOW_DEVICE_CONFIGURATION          (1 << 7)
#define CSR_ALLOW_ANY_RECOVERY_OS               (1 << 8)
#define CSR_ALLOW_UNAPPROVED_KEXTS              (1 << 9)
#define CSR_ALLOW_EXECUTABLE_POLICY_OVERRIDE    (1 << 10)
#define CSR_ALLOW_UNAUTHENTICATED_ROOT          (1 << 11) // macOS 11+

#endif

C_API bool fs_is_system_integrity_protection_enabled(void)
{
#if __APPLE__
    return csr_check(CSR_ALLOW_UNRESTRICTED_FS) == 1;
#else
    return false;
#endif
}
