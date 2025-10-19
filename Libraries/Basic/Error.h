#pragma once

#include "./Types.h"
#include "./Hash.h"

typedef enum ksystem : u8 {
    ksystem_unix,
    ksystem_au,
    ksystem_cli,
    ksystem_core,
    ksystem_dsp,
    ksystem_fs,
    ksystem_gl,
    ksystem_image,
    ksystem_layout,
    ksystem_library,
    ksystem_ms,
    ksystem_midi,
    ksystem_tar,
    ksystem_ty,
    ksystem_vst,
    ksystem_wasm,
} ksystem;

typedef enum : u8 {
    kcode_au_could_not_decode,
} kcode_au;
typedef enum : u8 {
    kcode_cli_dummy,
} kcode_cli;
typedef enum : u8 {
    kcode_core_dummy,
} kcode_core;
typedef enum : u8 {
    kcode_dsp_dummy,
} kcode_dsp;
typedef enum : u8 {
    kcode_fs_file_not_found,
} kcode_fs;
typedef enum : u8 {
    kcode_gl_could_not_compile_vertex_shader,
    kcode_gl_could_not_compile_fragment_shader,
    kcode_gl_could_not_link,
} kcode_gl;
typedef enum : u8 {
    kcode_image_dummy,
} kcode_image;
typedef enum : u8 {
    kcode_layout_dummy,
} kcode_layout;
typedef enum : u8 {
    kcode_library_dummy,
} kcode_library;
typedef enum : u8 {
    kcode_ms_dummy,
} kcode_ms;
typedef enum : u8 {
    kcode_midi_dummy,
} kcode_midi;
typedef enum : u8 {
    kcode_tar_dummy,
} kcode_tar;
typedef enum : u8 {
    kcode_ty_could_not_decode,
} kcode_ty;
typedef enum : u8 {
    kcode_vst_dummy,
} kcode_vst;
typedef enum : u8 {
    kcode_wasm_dummy,
} kcode_wasm;

typedef struct ksystem_code {
    alignas(u32) ksystem system;
    u8 : 8;
    union {
        u16 unix;
        kcode_au au;
        kcode_cli cli;
        kcode_core core;
        kcode_dsp dsp;
        kcode_fs fs;
        kcode_gl gl;
        kcode_image image;
        kcode_layout layout;
        kcode_library library;
        kcode_ms ms;
        kcode_midi midi;
        kcode_tar tar;
        kcode_ty ty;
        kcode_vst vst;
        kcode_wasm wasm;
    };
} ksystem_code;
static_assert(sizeof(ksystem_code) == sizeof(u32));
static_assert(alignof(ksystem_code) == alignof(u32));

typedef struct [[nodiscard]] KError {
    union {
        c_string function;
        u64 : 64;
    };
    u16 file;
    u16 line : 15;
    bool ok : 1;

    ksystem_code code;

#ifdef __cplusplus
    constexpr bool has_value() const { return ok; }
    constexpr void release_value() const {}
    constexpr KError release_error() const { return *this; }

    constexpr explicit operator bool() const { return !ok; }
#endif
} KError;
static const KError kerror_none = (KError) {
    .function = 0,
    .file = 0,
    .line = 0,
    .ok = true,
    .code = {
        .system = (ksystem)0,
        .unix = 0,
    },
};

C_EXTERN c_string kerror_file_names[0xFFFF];
C_INLINE u16 kerror_register_file(char const* name, u32 len)
{
    u16 hash = djb2(djb2_initial_seed, name, len) & 0xFFFF;
    kerror_file_names[hash] = name;
    return hash;
}

C_INLINE KError kerror_impl(c_string file_name, u16 file_name_len, u16 line, c_string function_name, ksystem_code code)
{
    return (KError) {
        .function = function_name,
        .file = kerror_register_file(file_name, file_name_len),
        .line = line,
        .ok = false,
        .code = code,
    };
}
#define ksystem_code(__system, __code) ((ksystem_code){ .system = ksystem_##__system, .__system = (__code) })
#define kerror(system, code) kerror_impl(__FILE_NAME__, sizeof(__FILE_NAME__) - 1, __LINE__, __FUNCTION__, ksystem_code(system, code))
#define kerror_unix(errno)      kerror(unix, (u16)(errno))
#define kerror_au(code)         kerror(au, code)
#define kerror_cli(code)        kerror(cli, code)
#define kerror_core(code)       kerror(core, code)
#define kerror_dsp(code)        kerror(dsp, code)
#define kerror_fs(code)         kerror_fs(fs, code)
#define kerror_gl(code)         kerror(gl, code)
#define kerror_image(code)      kerror(image, code)
#define kerror_layout(code)     kerror(layout, code)
#define kerror_library(code)    kerror(library, code)
#define kerror_ms(code)         kerror(ms, code)
#define kerror_midi(code)       kerror(midi, code)
#define kerror_tar(code)        kerror(tar, code)
#define kerror_ty(code)         kerror(ty, code)
#define kerror_vst(code)        kerror(vst, code)
#define kerror_wasm(code)       kerror(wasm, code)

C_INLINE c_string kerror_file(KError error) { return kerror_file_names[error.file]; }
C_INLINE u16 kerror_line(KError error) { return error.line; }
C_INLINE c_string kerror_function(KError error) { return error.function; }
C_INLINE u32 kcode(KError error)
{
    static_assert(sizeof(error.code) == sizeof(u32));
    static_assert(alignof(ksystem_code) == alignof(u32));
    return __builtin_bit_cast(u32, error.code);
}

#ifdef __cplusplus
constexpr u32 kcode(ksystem system, u16 n)
{
    struct inner {
        ksystem system;
        u8 pad;
        u16 n;
    };
    inner inner;
    inner.system = system;
    inner.pad = 0;
    inner.n = n;
    return __builtin_bit_cast(u32, inner);
}

constexpr u32 kcode(u16 code)           { return kcode(ksystem_unix, code);      }
constexpr u32 kcode(kcode_au code)      { return kcode(ksystem_au, code);        }
constexpr u32 kcode(kcode_cli code)     { return kcode(ksystem_cli, code);       }
constexpr u32 kcode(kcode_core code)    { return kcode(ksystem_core, code);      }
constexpr u32 kcode(kcode_fs code)      { return kcode(ksystem_fs, code);        }
constexpr u32 kcode(kcode_gl code)      { return kcode(ksystem_gl, code);        }
constexpr u32 kcode(kcode_image code)   { return kcode(ksystem_image, code);     }
constexpr u32 kcode(kcode_layout code)  { return kcode(ksystem_layout, code);    }
constexpr u32 kcode(kcode_library code) { return kcode(ksystem_library, code);   }
constexpr u32 kcode(kcode_ms code)      { return kcode(ksystem_ms, code);        }
constexpr u32 kcode(kcode_midi code)    { return kcode(ksystem_midi, code);      }
constexpr u32 kcode(kcode_tar code)     { return kcode(ksystem_tar, code);       }
constexpr u32 kcode(kcode_ty code)      { return kcode(ksystem_ty, code);        }
constexpr u32 kcode(kcode_vst code)     { return kcode(ksystem_vst, code);       }
constexpr u32 kcode(kcode_wasm code)    { return kcode(ksystem_wasm, code);      }
#endif

C_API c_string kerror_strsystem(KError error);
C_API c_string kerror_strerror(KError error);

C_API u64 kerror_message_buffer_min_size(KError);
C_API c_string kerror_string(KError, char* buf, u64 buf_size);
C_API c_string kerror_tstring(KError);
