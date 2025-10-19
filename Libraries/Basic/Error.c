#include "./Error.h"

#include "./FixedArena.h"
#include "./Context.h"
#include "Format.h"

#include <string.h>

static c_string kcode_au_strerror(kcode_au);
static c_string kcode_cli_strerror(kcode_cli);
static c_string kcode_core_strerror(kcode_core);
static c_string kcode_dsp_strerror(kcode_dsp);
static c_string kcode_fs_strerror(kcode_fs);
static c_string kcode_gl_strerror(kcode_gl);
static c_string kcode_image_strerror(kcode_image);
static c_string kcode_layout_strerror(kcode_layout);
static c_string kcode_library_strerror(kcode_library);
static c_string kcode_ms_strerror(kcode_ms);
static c_string kcode_midi_strerror(kcode_midi);
static c_string kcode_tar_strerror(kcode_tar);
static c_string kcode_ty_strerror(kcode_ty);
static c_string kcode_vst_strerror(kcode_vst);
static c_string kcode_wasm_strerror(kcode_wasm);

c_string kerror_file_names[0xFFFF] = { 0 };

C_API c_string kerror_strsystem(KError error)
{
    switch (error.code.system) {
    case ksystem_unix:    return "unix";
    case ksystem_au:      return "au";
    case ksystem_cli:     return "cli";
    case ksystem_core:    return "core";
    case ksystem_dsp:     return "dsp";
    case ksystem_fs:      return "fs";
    case ksystem_gl:      return "gl";
    case ksystem_image:   return "image";
    case ksystem_layout:  return "layout";
    case ksystem_library: return "library";
    case ksystem_ms:      return "ms";
    case ksystem_midi:    return "midi";
    case ksystem_tar:     return "tar";
    case ksystem_ty:      return "ty";
    case ksystem_vst:     return "vst";
    case ksystem_wasm:    return "wasm";
    }
}

C_API c_string kerror_strerror(KError error)
{
    switch (error.code.system) {
    case ksystem_unix:    return strerror(error.code.unix);
    case ksystem_au:      return kcode_au_strerror(error.code.au);
    case ksystem_cli:     return kcode_cli_strerror(error.code.cli);
    case ksystem_core:    return kcode_core_strerror(error.code.core);
    case ksystem_dsp:     return kcode_dsp_strerror(error.code.dsp);
    case ksystem_fs:      return kcode_fs_strerror(error.code.fs);
    case ksystem_gl:      return kcode_gl_strerror(error.code.gl);
    case ksystem_image:   return kcode_image_strerror(error.code.image);
    case ksystem_layout:  return kcode_layout_strerror(error.code.layout);
    case ksystem_library: return kcode_library_strerror(error.code.library);
    case ksystem_ms:      return kcode_ms_strerror(error.code.ms);
    case ksystem_midi:    return kcode_midi_strerror(error.code.midi);
    case ksystem_tar:     return kcode_tar_strerror(error.code.tar);
    case ksystem_ty:      return kcode_ty_strerror(error.code.ty);
    case ksystem_vst:     return kcode_vst_strerror(error.code.vst);
    case ksystem_wasm:    return kcode_wasm_strerror(error.code.wasm);
    }
}

C_API u64 kerror_message_buffer_min_size(KError error)
{
    c_string system = kerror_strsystem(error);
    c_string function = kerror_function(error);
    c_string message = kerror_strerror(error);
    c_string file = kerror_file(error);
    u16 line = kerror_line(error);
    return format_size_including_null("%s: %s [%s][%s:%u]", function, message, system, file, line);
}

C_API c_string kerror_string(KError error, char* buf, u64 buf_size)
{
    FixedArena arena = fixed_arena_init(buf, buf_size);
    c_string system = kerror_strsystem(error);
    c_string function = kerror_function(error);
    c_string message = kerror_strerror(error);
    c_string file = kerror_file(error);
    u16 line = kerror_line(error);
    return fixed_arena_fmt(&arena, "%s: %s [%s][%s:%u]", function, message, system, file, line);
}

C_API c_string kerror_tstring(KError error)
{
    u64 size = kerror_message_buffer_min_size(error);
    char* buf = (char*)memalloc(temporary_arena(), size, 1);
    if (!buf) return kerror_strerror(error);
    return kerror_string(error, buf, size);
}

static c_string kcode_au_strerror(kcode_au code)
{
    switch (code) {
    case kcode_au_could_not_decode: return "could not decode";
    }
}

static c_string kcode_cli_strerror(kcode_cli code)
{
    switch (code) {
    case kcode_cli_dummy: return "dummy";
    }
}

static c_string kcode_core_strerror(kcode_core code)
{
    switch (code) {
    case kcode_core_dummy: return "dummy";
    }
}

static c_string kcode_dsp_strerror(kcode_dsp code)
{
    switch (code) {
    case kcode_dsp_dummy: return "dummy";
    }
}

static c_string kcode_fs_strerror(kcode_fs code)
{
    switch (code) {
    case kcode_fs_file_not_found: return "file not found";
    }
}

static c_string kcode_gl_strerror(kcode_gl code)
{
    switch (code) {
    case kcode_gl_could_not_compile_vertex_shader: return "could not compile vertex shader";
    case kcode_gl_could_not_compile_fragment_shader: return "could not compile fragment shader";
    case kcode_gl_could_not_link: return "could not link program";
    }
}

static c_string kcode_image_strerror(kcode_image code)
{
    switch (code) {
    case kcode_image_dummy: return "dummy";
    }
}

static c_string kcode_layout_strerror(kcode_layout code)
{
    switch (code) {
    case kcode_layout_dummy: return "dummy";
    }
}

static c_string kcode_library_strerror(kcode_library code)
{
    switch (code) {
    case kcode_library_dummy: return "dummy";
    }
}

static c_string kcode_ms_strerror(kcode_ms code)
{
    switch (code) {
    case kcode_ms_dummy: return "dummy";
    }
}

static c_string kcode_midi_strerror(kcode_midi code)
{
    switch (code) {
    case kcode_midi_dummy: return "dummy";
    }
}

static c_string kcode_tar_strerror(kcode_tar code)
{
    switch (code) {
    case kcode_tar_dummy: return "dummy";
    }
}

static c_string kcode_ty_strerror(kcode_ty code)
{
    switch (code) {
    case kcode_ty_could_not_decode: return "could not decode";
    }
}

static c_string kcode_vst_strerror(kcode_vst code)
{
    switch (code) {
    case kcode_vst_dummy: return "dummy";
    }
}

static c_string kcode_wasm_strerror(kcode_wasm code)
{
    switch (code) {
    case kcode_wasm_dummy: return "dummy";
    }
}
