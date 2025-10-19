#include "./VirtualMachine.h"

#include "./Internal/VirtualMachine.h"
#include "./Forward.h"
#include "Internal/Opcode.h"

#include <Basic/Types.h>
#include <Basic/ByteDecoder.h>
#include <Basic/Array.h>
#include <Basic/Verify.h>
#include <Basic/Context.h>
#include <Basic/StringSlice.h>
#include <Basic/Try.h>

#include <string.h>

static KError wasm_expect_valtype(ByteDecoder*, WASMValType*);
static KError wasm_expect_resulttype(ByteDecoder*, WASMResultType*);
static KError wasm_expect_functype(ByteDecoder*, WASMFuncType*);
static KError wasm_expect_idx(ByteDecoder*, u32*, c_string name);
static KError wasm_expect_typeidx(ByteDecoder*, WASMTypeIdx*);
static KError wasm_expect_funcidx(ByteDecoder*, WASMFuncIdx*);
static KError wasm_expect_tableidx(ByteDecoder*, WASMTableIdx*);
static KError wasm_expect_memidx(ByteDecoder*, WASMMemIdx*);
static KError wasm_expect_globalidx(ByteDecoder*, WASMGlobalIdx*);
// static KError wasm_expect_elemidx(ByteDecoder*, WASMElemIdx*);
// static KError wasm_expect_dataidx(ByteDecoder*, WASMDataIdx*);
// static KError wasm_expect_localidx(ByteDecoder*, WASMLocalIdx*);
// static KError wasm_expect_labelidx(ByteDecoder*, WASMLabelIdx*);
static KError wasm_expect_limits(ByteDecoder*, WASMLimits*);
static KError wasm_expect_memtype(ByteDecoder*, WASMMemType*);
static KError wasm_expect_externtype(ByteDecoder*, WASMExternType*);
static KError wasm_expect_name(ByteDecoder*, WASMName*);
static KError wasm_expect_export_desc(ByteDecoder*, WASMExportDesc*);
static KError wasm_expect_globaltype(ByteDecoder*, WASMGlobalType*);
static KError wasm_expect_constinitexpr(ByteDecoder*, WASMConstantInitializerExpression*);
static KError wasm_expect_global(ByteDecoder*, WASMGlobal*);
static KError wasm_expect_export(ByteDecoder*, WASMExport*);
static KError wasm_expect_code(ByteDecoder*, WASMCode*);
static KError wasm_expect_locals(ByteDecoder*, WASMLocals*);
static KError wasm_expect_locals_vector(ByteDecoder*, WASMLocalsVector*);
static KError wasm_expect_expression(ByteDecoder*, WASMExpression*);
static KError wasm_expect_import(ByteDecoder*, WASMImport*);
static KError wasm_expect_data_segment(ByteDecoder*, Bytes* apply, u64* apply_at_index);

static KError wasm_expect_type_section(ByteDecoder*, WASMTypeSection*);
static KError wasm_expect_import_section(ByteDecoder*, WASMImportSection*);
static KError wasm_expect_function_section(ByteDecoder*, WASMFunctionSection*);
static KError wasm_expect_memory_section(ByteDecoder*, WASMMemorySection*);
static KError wasm_expect_global_section(ByteDecoder*, WASMGlobalSection*);
static KError wasm_expect_export_section(ByteDecoder*, WASMExportSection*);
static KError wasm_expect_code_section(ByteDecoder*, WASMCodeSection*);
static KError wasm_expect_data_section(ByteDecoder*, WASMMemory* memory);

static KError wasm_expect_opcode(ByteDecoder*, WASMOpcode*);

C_API u64 wasm_mod_min_size(void) { return sizeof(WASMModule); }
C_API u64 wasm_mod_min_align(void) { return alignof(WASMModule); }
/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.5.16
/// 5.5.16 Modules
C_API KError wasm_mod_init(WASMModule** out, void* module_memory, u64 module_memory_size, u64 module_memory_align, void const* wasm_data, u64 wasm_size)
{
    if (module_memory_size < sizeof(WASMModule)) {
        errorf("module_memory has insufficient space for WASMModule");
        return kerror_unix(EINVAL);
    }
    if (module_memory_align < alignof(WASMModule)) {
        errorf("module_memory has insufficient alignment for WASMModule");
        return kerror_unix(EINVAL);
    }
    if (!module_memory) {
        errorf("module_memory was null");
        return kerror_unix(EINVAL);
    }

    WASMModule* module = (WASMModule*)module_memory;
    memzero(module, sizeof(*module));

    ByteDecoder decoder = byte_decoder(bytes(wasm_data, wasm_size));

    WASMTypeSection type_section = { 0 };
    WASMImportSection import_section = { 0 };
    WASMFunctionSection function_section = { 0 };
    WASMMemorySection memory_section = { 0 };
    WASMGlobalSection global_section = { 0 };
    WASMExportSection export_section = { 0 };
    // WASMCodeSection code_section = { 0 };

    /// The encoding of a module starts with a preamble containing a 4-byte magic number (the string ‘∖0asm’)
    StringSlice magic = sv_from_parts("\0asm", 4);
    if (!byte_expect_string(&decoder, magic).ok) {
        errorf("wasm magic mismatch");
        return kerror_unix(EINVAL);
    }

    /// and a version field. The current version of the WebAssembly binary format is 1.
    if (!byte_expect_u32le(&decoder, 1).ok) {
        errorf("wasm version was not 1");
        return kerror_unix(EINVAL);
    }

    /// The preamble is followed by a sequence of sections.
    /// Custom sections may be inserted at any place in this sequence,
    /// while other sections must occur at most once and in the prescribed order.
    /// All sections can be empty.
    u8 highest_section = 0;

    /// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.5.2
    /// 5.5.2 Sections
    while (byte_peek_u8(&decoder, nullptr).found) {
        /// Each section consists of
 
        /// * a one-byte section id,
        u8 section_id = 0;
        if (!byte_parse_u8(&decoder, &section_id).found) {
            errorf("could not find section id");
            return kerror_unix(EINVAL);
        }

        /// * the u32 size of the contents, in bytes,
        u64 content_size = 0;
        if (!byte_parse_uleb128(&decoder, &content_size).found) {
            errorf("section did not include content size");
            return kerror_unix(EINVAL);
        }

        /// * the actual contents, whose structure is dependent on the section id.
        Bytes actual_content_bytes = {};
        if (!byte_parse_bytes(&decoder, content_size, &actual_content_bytes).found) {
            errorf("could not parse section content");
            return kerror_unix(EINVAL);
        }
        ByteDecoder actual_content = byte_decoder(actual_content_bytes);

        if (section_id != WASMSectionID_Custom && highest_section >= section_id) {
            errorf("section has already been defined or is not in the right order");
            return kerror_unix(EINVAL);
        }
        highest_section = section_id;
        switch (section_id) {
        case WASMSectionID_Custom:
            continue;
        case WASMSectionID_TypeSection: {
            debugf("type section");
            TRY(wasm_expect_type_section(&actual_content, &type_section));
            for (u64 i = 0; i < type_section.count; i++) {
                WASMFuncType item = type_section.items[i];
                debugf("- functype");
                debugf("  - parameters=%u", item.parameters.count);
                for (u64 parameter = 0; parameter < item.parameters.count; parameter++) {
                    WASMValType valtype = item.parameters.items[parameter];
                    debugf("    - %s (0x%.2X)", wasm_valtype_string(valtype), valtype);
                }
                debugf("  - result_types=%u", item.result_types.count);
                for (u64 result_type = 0; result_type < item.result_types.count; result_type++) {
                    WASMValType valtype = item.result_types.items[result_type];
                    debugf("    - %s (0x%.2X)", wasm_valtype_string(valtype), valtype);
                }
            }
            VERIFY(actual_content.cursor == actual_content.bytes.count);
            continue;
        }
        case WASMSectionID_ImportSection: {
            debugf("import section");
            TRY(wasm_expect_import_section(&actual_content, &import_section));
            for (u64 i = 0; i < import_section.count; i++) {
                WASMImport item = import_section.items[i];
                debugf("- %lu mod='%.*s' name='%.*s' %s=%d",
                    i,
                    item.mod.count,
                    item.mod.items,
                    item.name.count,
                    item.name.items,
                    wasm_externtype_tag_string(item.externtype.tag),
                    item.externtype.raw_index
                );
            }
            VERIFY(actual_content.cursor == actual_content.bytes.count);
            continue;
        }
        case WASMSectionID_FunctionSection: {
            debugf("function section");
            TRY(wasm_expect_function_section(&actual_content, &function_section));
            for (u64 i = 0; i < function_section.count; i++) {
                WASMTypeIdx item = function_section.items[i];
                debugf("- typeidx=%u", item.value);
            }
            VERIFY(actual_content.cursor == actual_content.bytes.count);
            continue;
        }
        case WASMSectionID_TableSection: {
            warnf("table section");
            continue;
        }
        case WASMSectionID_MemorySection: {
            debugf("memory section");
            TRY(wasm_expect_memory_section(&actual_content, &memory_section));
            for (u64 i = 0; i < memory_section.count; i++) {
                WASMMemType item = memory_section.items[i];
                debugf("- memtype min=%u(%luKi) max=%u(%luKi)",
                    item.limits.min, item.limits.min * wasm_page_size / 1024,
                    item.limits.max, item.limits.max * wasm_page_size / 1024
                );
            }
            VERIFY(actual_content.cursor == actual_content.bytes.count);
            continue;
        }
        case WASMSectionID_GlobalSection: {
            debugf("global section");
            TRY(wasm_expect_global_section(&actual_content, &global_section));
            for (u64 i = 0; i < global_section.count; i++) {
                WASMGlobal item = global_section.items[i];
                debugf("- global valtype=%s(0x%.2X) is_mutable=%s",
                    wasm_valtype_string(item.type.valtype), item.type.valtype,
                    item.type.is_mutable ? "true" : "false"
                );
                // FIXME: Log initializer
            }
            continue;
        }
        case WASMSectionID_ExportSection: {
            debugf("export section");
            TRY(wasm_expect_export_section(&actual_content, &export_section));
            for (u64 i = 0; i < export_section.count; i++) {
                WASMExport item = export_section.items[i];
                debugf("- kind=%6s(%u) idx=%u name='%.*s'",
                    wasm_export_desc_kind_string(item.desc.kind), item.desc.kind,
                    item.desc.raw_index,
                    item.name.count, item.name.items
                );
            }
            VERIFY(actual_content.cursor == actual_content.bytes.count);
            continue;
        }
        case WASMSectionID_StartSection: {
            warnf("start section");
            continue;
        }
        case WASMSectionID_ElementSection: {
            warnf("element section");
            continue;
        }
        case WASMSectionID_CodeSection: {
            debugf("code section");
            memzero(&module->code_section, sizeof(module->code_section));
            TRY(wasm_expect_code_section(&actual_content, &module->code_section));
            for (u64 code_index = 0; code_index < module->code_section.count; code_index++) {
                WASMCode code = module->code_section.items[code_index];
                debugf("- code=%lu size=%lu", code_index, code.size);

                for (u64 locals_index = 0; locals_index < code.locals.count; locals_index++) {
                    WASMLocals locals = code.locals.items[locals_index];
                    debugf("  - local type=%s(%u) count=%u",
                        wasm_valtype_string(locals.type),
                        locals.type,
                        locals.count
                    );
                }

                for (u64 instruction_index = 0; instruction_index < code.expression.count; instruction_index++) {
                    WASMInstruction instruction = code.expression.items[instruction_index];
                    debugf("  - %s(0x%0.2lX) %ld %ld",
                        wasm_opcode_string(instruction.opcode),
                        instruction.opcode,
                        instruction.a.u64,
                        instruction.b.u64
                    );
                }
            }
            infof("%ld:%ld", actual_content.cursor, actual_content.bytes.count);
            VERIFY(actual_content.cursor == actual_content.bytes.count);
            continue;
        }
        case WASMSectionID_DataSection: {
            debugf("data section");
            memzero(&module->memory, sizeof(module->memory));
            TRY(wasm_expect_data_section(&actual_content, &module->memory));
            continue;
        }
        case WASMSectionID_DataCountSection: {
            warnf("data count section");
            continue;
        }
        default:
            warnf("unknown section %u", section_id);
            continue;
        }
    }

    module->type_section = type_section;
    module->import_section = import_section;
    module->function_section = function_section;
    module->memory_section = memory_section;
    module->global_section = global_section;
    module->export_section = export_section;
    // vm->code_section = code_section;
    *out = module;
    return kerror_none;
}

C_API u64 wasm_vm_min_size() { return sizeof(WASMVirtualMachine); }
C_API u64 wasm_vm_min_align() { return alignof(WASMVirtualMachine); }

C_API KError wasm_vm_init(WASMVirtualMachine** out, void* vm_memory, u64 vm_memory_size, u64 vm_memory_align, WASMModule const* module)
{
    if (vm_memory_size < sizeof(WASMModule)) {
        errorf("vm_memory has insufficient space for WASMVirtualMachine");
        return kerror_unix(EINVAL);
    }
    if (vm_memory_align < alignof(WASMModule)) {
        errorf("vm_memory has insufficient alignment for WASMVirtualMachine");
        return kerror_unix(EINVAL);
    }
    if (!vm_memory) {
        errorf("vm_memory was null");
        return kerror_unix(EINVAL);
    }

    WASMVirtualMachine* vm = (WASMVirtualMachine*)vm_memory;
    memzero(vm, sizeof(*module));

    vm->module = module;
    vm->globals = module->globals;
    vm->memory = module->memory;
    *out = vm;
    return kerror_none;
}

static KError wasm_push_b32(WASMVirtualMachine* vm, void* value)
{
    return static_push(&vm->stack, *(u32*)value);
}

static void* wasm_pop_b32(WASMVirtualMachine* vm)
{
    if (vm->stack.count == 0)
        return 0;
    return &vm->stack.items[--vm->stack.count];
}

C_API KError wasm_vm_push_i32(WASMVirtualMachine* vm, i32 value)
{
    return wasm_push_b32(vm, &value);
}


C_API KError wasm_vm_push_u32(WASMVirtualMachine* vm, u32 value)
{
    return wasm_push_b32(vm, &value);
}

C_API KError wasm_vm_pop_i32(WASMVirtualMachine* vm, i32* out)
{
    i32* addr = (i32*)wasm_pop_b32(vm);
    if (addr == nullptr)
        return kerror_unix(EFAULT);
    if (out) *out = *addr;
    return kerror_none;
}

C_API KError wasm_vm_pop_u32(WASMVirtualMachine* vm, u32* out)
{
    u32* addr = (u32*)wasm_pop_b32(vm);
    if (addr == nullptr)
        return kerror_unix(EFAULT);
    if (out) *out = *addr;
    return kerror_none;
}

C_API KError wasm_vm_push_f32(WASMVirtualMachine* vm, f32 value)
{
    return wasm_push_b32(vm, &value);
}

C_API KError wasm_vm_pop_f32(WASMVirtualMachine* vm, f32* out)
{
    f32* addr = (f32*)wasm_pop_b32(vm);
    if (addr == nullptr)
        return kerror_unix(EFAULT);
    if (out) *out = *addr;
    return kerror_none;
}

C_API KError wasm_vm_function(WASMVirtualMachine* vm, c_string name, WASMFunctionID* out)
{
    WASMModule const* mod = vm->module;
    for (u64 i = 0; i < mod->export_section.count; i++) {
        WASMExport export = mod->export_section.items[i];
        if (export.desc.kind != WASMExportDescKind_Func)
            continue;
        if (strncmp(name, name, export.name.count) != 0)
            continue;
        *out = (WASMFunctionID){
            .index = export.desc.func.value,
        };
        return kerror_none;
    }
    return kerror_unix(ENOENT);
}

C_API KError wasm_mod_bind_variable(WASMModule* vm, c_string name, void* variable)
{
    return static_push(&vm->native_variables, (WASMNativeVariable) {
        .name = name,
        .address = variable
    });
}


static bool wasm_mod_find_function_import(WASMModule const* module, c_string mod, c_string name, u64* out)
{
    for (u64 i = 0; i < module->import_section.count; i++) {
        WASMImport const* import = &module->import_section.items[i];
        if (import->externtype.tag != WASMExternTypeTag_typeidx)
            continue;
        if (strncmp(mod, import->mod.items, import->mod.count) != 0)
            continue;
        if (strncmp(name, import->name.items, import->name.count) != 0)
            continue;
        *out = i;
        return true;
    }
    return false;
}


static KError parse_signature(c_string sig, u32* inputs, u32* outputs)
{
    u64 len = strlen(sig);
    i32 in = 0;
    i32 out = 0;
    char expect_paren = '(';
    u64 i = 0;
    for (i = 0; i < len; i++) {
        switch (sig[i]) {
        case 'i':
        case 'f':
        case '*':
            if (expect_paren == '(') out += 1;
            if (expect_paren == ')') in += 1;
            continue;

        case '(':
            if (expect_paren != '(') {
                errorf("did not expect opening parenthesis at index %ld", i);
                return kerror_unix(EINVAL);
            }
            expect_paren = ')';
            continue;
        case ')':
            if (expect_paren != ')') {
                errorf("did not expect closing parenthesis at index %ld", i);
                return kerror_unix(EINVAL);
            }
            goto eof;

        case 'I':
        case 'F':
        case 'v':
        default:
            errorf("unsupported %s type: %c", expect_paren == '(' ? "return" : "operand", sig[i]);
            return kerror_unix(EINVAL);
        }
    }
eof:
    if (i != len - 1) {
        errorf("found characters after closing parenthesis");
        return kerror_unix(EINVAL);
    }

    *inputs = in;
    *outputs = out;
    return kerror_none;
}


C_API KError wasm_mod_bind_function(WASMModule* module, c_string mod, c_string name, c_string signature, WASMNativeFunctionCallback callback)
{
    u64 index = 0;
    if (!wasm_mod_find_function_import(module, mod, name, &index))
        return kerror_none;
    if (index >= ARRAY_SIZE(module->native_functions))
        return kerror_unix(ENOMEM);
    u32 inputs = 0;
    u32 outputs = 0;
    TRY(parse_signature(signature, &inputs, &outputs));
    module->native_functions[index] = (WASMNativeFunction) {
        .callback = callback,
        .mod = mod,
        .name = name,
        .inputs = inputs,
        .outputs = outputs,
    };
    return kerror_none;
}

C_API void* wasm_vm_variable(WASMVirtualMachine* vm, c_string name)
{
    WASMModule const* module = vm->module;
    for (u64 i = 0; i < module->native_variables.count; i++) {
        WASMNativeVariable const* var = &module->native_variables.items[i];
        if (strcmp(var->name, name) == 0)
            return var->address;
    }
    return 0;
}

C_API void* wasm_vm_resolve_addr(WASMVirtualMachine* context, u32 addr, u32* size_until_end)
{
    if (addr >= ARRAY_SIZE(context->memory.items))
        return nullptr;
    if (size_until_end) *size_until_end = ARRAY_SIZE(context->memory.items) - addr;
    return &context->memory.items[addr];
}

/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.3.4
/// 5.3.4 Value Types
/// Value types are encoded with their respective encoding as a number type, vector type, or reference type.
static KError wasm_expect_valtype(ByteDecoder* decoder, WASMValType* out)
{
    ByteDecoder d = *decoder;

    u8 raw_kind = 0;
    if (!byte_parse_u8(&d, &raw_kind).found) {
        errorf("expected valtype");
        return kerror_unix(EINVAL);
    }
    switch (raw_kind) {
    case WASMValType_i32:
    case WASMValType_i64:
    case WASMValType_f32:
    case WASMValType_f64:
    case WASMValType_v128:
    case WASMValType_FuncRef:
    case WASMValType_ExternRef:
        *decoder = d;
        *out = raw_kind;
        return kerror_none;
    default:
        errorf("invalid valtype (0x%.2X)", raw_kind);
        return kerror_unix(EINVAL);
    }
}

/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.3.5
/// 5.3.5 Result Types
/// Result types are encoded by the respective vectors of value types.
static KError wasm_expect_resulttype(ByteDecoder* decoder, WASMResultType* out)
{
    ByteDecoder d = *decoder;
    WASMResultType result = { 0 };

    /// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.1.3
    /// 5.1.3 Vectors
    /// Vectors are encoded with their u32 length followed by the encoding of their element sequence.

    u64 vector_elements = 0;
    if (!byte_parse_uleb128(&d, &vector_elements).found) {
        errorf("resulttype did not say how large the vector is");
        return kerror_unix(EINVAL);
    }
    if (vector_elements > static_size_left(out)) {
        errorf("too many result types");
        return kerror_unix(EINVAL);
    }

    for (u64 i = 0; i < vector_elements; i++) {
        WASMValType value = { 0 };
        TRY(wasm_expect_valtype(&d, &value));
        TRY(static_push(&result, value));
    }

    *decoder = d;
    *out = result;
    return kerror_none;
}

/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.3.6
/// 5.3.6 Function Types
/// Function types are encoded by the byte 0x60 followed by the respective vectors of parameter and result types.
static KError wasm_expect_functype(ByteDecoder* decoder, WASMFuncType* out)
{
    ByteDecoder d = *decoder;

    if (!byte_expect_u8(&d, 0x60).ok) {
        errorf("expected functype (0x60)");
        return kerror_unix(EINVAL);
    }

    WASMResultType parameters = { 0 };
    TRY(wasm_expect_resulttype(&d, &parameters));

    WASMResultType result_types = { 0 };
    TRY(wasm_expect_resulttype(&d, &result_types));

    *decoder = d;
    *out = (WASMFuncType){
        .parameters = parameters,
        .result_types = result_types,
    };
    return kerror_none;
}


/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.5.1
/// 5.5.1 Indices
/// All indices are encoded with their respective value.
static KError wasm_expect_idx(ByteDecoder* decoder, u32* out, c_string name)
{
    ByteDecoder d = *decoder;

    u64 value = 0;
    if (!byte_parse_uleb128(&d, &value).found) {
        errorf("expected %s", name);
        return kerror_unix(EINVAL);
    }
    u32 truncated_value = value;
    if (truncated_value != value) {
        errorf("%s does not fit in a u32", name);
        return kerror_unix(EINVAL);
    }

    *decoder = d;
    *out = truncated_value;
    return kerror_none;
}

static KError wasm_expect_typeidx(ByteDecoder* decoder, WASMTypeIdx* out)       { return wasm_expect_idx(decoder, &out->value, "typeidx");      }
static KError wasm_expect_funcidx(ByteDecoder* decoder, WASMFuncIdx* out)       { return wasm_expect_idx(decoder, &out->value, "funcidx");      }
static KError wasm_expect_tableidx(ByteDecoder* decoder, WASMTableIdx* out)     { return wasm_expect_idx(decoder, &out->value, "tableidx");     }
static KError wasm_expect_memidx(ByteDecoder* decoder, WASMMemIdx* out)         { return wasm_expect_idx(decoder, &out->value, "memidx");       }
static KError wasm_expect_globalidx(ByteDecoder* decoder, WASMGlobalIdx* out)   { return wasm_expect_idx(decoder, &out->value, "globalidx");    }
// static KError wasm_expect_elemidx(ByteDecoder* decoder, WASMElemIdx* out)       { return wasm_expect_idx(decoder, &out->value, "elemidx");      }
// static KError wasm_expect_dataidx(ByteDecoder* decoder, WASMDataIdx* out)       { return wasm_expect_idx(decoder, &out->value, "dataidx");      }
// static KError wasm_expect_localidx(ByteDecoder* decoder, WASMLocalIdx* out)     { return wasm_expect_idx(decoder, &out->value, "localidx");     }
// static KError wasm_expect_labelidx(ByteDecoder* decoder, WASMLabelIdx* out)     { return wasm_expect_idx(decoder, &out->value, "labelidx");     }
static KError wasm_expect_tagidx(ByteDecoder* decoder, WASMTagIdx* out)     { return wasm_expect_idx(decoder, &out->value, "tagidx");     }


/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.2.3.7
/// 2.3.7 Limits
/// Limits classify the size range of resizeable storage associated with memory types and table types
static KError wasm_expect_limits(ByteDecoder* decoder, WASMLimits* out)
{
    ByteDecoder d = *decoder;

    u64 min = 0;
    if (!byte_parse_uleb128(&d, &min).found) {
        errorf("expected limits min value");
        return kerror_unix(EINVAL);
    }
    u32 truncated_min = min;
    if (truncated_min != min) {
        errorf("limits min value does not fit in u32");
        return kerror_unix(EINVAL);
    }

    u64 max = 0;
    if (!byte_parse_uleb128(&d, &max).found) {
        errorf("expected limits max value");
        return kerror_unix(EINVAL);
    }
    u32 truncated_max = max;
    if (truncated_max != max) {
        errorf("limits max value does not fit in u32");
        return kerror_unix(EINVAL);
    }

    *decoder = d;
    *out = (WASMLimits){
        .min = truncated_min,
        .max = truncated_max,
    };
    return kerror_none;
}


/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.2.3.8
/// Memory types classify linear memories and their size range
/// The limits constrain the minimum and optionally the maximum size of a memory. The limits are given in units of
/// page size
static KError wasm_expect_memtype(ByteDecoder* decoder, WASMMemType* out)
{
    WASMLimits limits = { 0 };
    TRY(wasm_expect_limits(decoder, &limits));
    *out = (WASMMemType){
        .limits = limits,
    };
    return kerror_none;
}


/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.2.2.5
/// 2.2.5 Names
/// Names are sequences of characters, which are scalar values as defined by Unicode (Section 2.4).
/// Due to the limitations of the binary format, the length of a name is bounded by the length of its UTF-8 encoding
static KError wasm_expect_name(ByteDecoder* decoder, WASMName* out)
{
    ByteDecoder d = *decoder;
    WASMName result = { 0 };

    /// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.1.3
    /// 5.1.3 Vectors
    /// Vectors are encoded with their u32 length followed by the encoding of their element sequence.
    u64 vector_elements = 0;
    if (!byte_parse_uleb128(&d, &vector_elements).found) {
        errorf("name did not have a size");
        return kerror_unix(EINVAL);
    }
    if (vector_elements > static_size_left(&result)) {
        errorf("name size is too large, was %lu, but max is %lu", vector_elements, static_size_left(&result));
        return kerror_unix(EINVAL);
    }

    StringSlice s = { 0 };
    if (!byte_parse_string(&d, vector_elements, &s).found) {
        errorf("expected name of length %lu, found %lu", vector_elements, s.count);
        return kerror_unix(EINVAL);
    }
    VERIFY(s.count <= ARRAY_SIZE(result.items));
    memcpy(result.items, s.items, s.count);
    result.count = s.count;

    *decoder = d;
    *out = result;
    return kerror_none;
}


/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.2.5.10
/// 2.5.10 Exports
static KError wasm_expect_export_desc(ByteDecoder* decoder, WASMExportDesc* out)
{
    ByteDecoder d = *decoder;
    WASMExportDesc result = { 0 };

    u8 raw_kind = 0;
    if (!byte_parse_u8(&d, &raw_kind).found) {
        errorf("expected export desc kind");
        return kerror_unix(EINVAL);
    }

    switch (raw_kind) {
    case WASMExportDescKind_Func:
        result.kind = raw_kind;
        TRY(wasm_expect_funcidx(&d, &result.func));
        break;
    case WASMExportDescKind_Table:
        result.kind = raw_kind;
        TRY(wasm_expect_tableidx(&d, &result.table));
        break;
    case WASMExportDescKind_Memory:
        result.kind = raw_kind;
        TRY(wasm_expect_memidx(&d, &result.mem));
        break;
    case WASMExportDescKind_Global:
        result.kind = raw_kind;
        TRY(wasm_expect_globalidx(&d, &result.global));
        break;
    default:
        errorf("invalid export desc kind (0x%.2X)", raw_kind);
        return kerror_unix(EINVAL);
    }

    *decoder = d;
    *out = result;
    return kerror_none;
}


/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.2.3.10
/// 2.3.10 Global Types
/// Global types classify global variables, which hold a value and can either be mutable or immutable.
static KError wasm_expect_globaltype(ByteDecoder* decoder, WASMGlobalType* out)
{
    ByteDecoder d = *decoder;

    WASMValType valtype = { 0 };
    TRY(wasm_expect_valtype(&d, &valtype));

    u8 raw_is_mutable = 0;
    if (!byte_parse_u8(&d, &raw_is_mutable).found) {
        errorf("expected type mutability");
        return kerror_unix(EINVAL);
    }

    bool is_mutable = false;
    switch (raw_is_mutable) {
    case 0:
        is_mutable = false;
        break;
    case 1:
        is_mutable = true;
        break;
    default:
        errorf("unknown mutability 0x%.2X", raw_is_mutable);
        return kerror_unix(EINVAL);
    }


    *decoder = d;
    *out = (WASMGlobalType){
        .valtype = valtype,
        .is_mutable = is_mutable,
    };
    return kerror_none;
}

// FIXME: Find spec for this.
static KError wasm_expect_opcode(ByteDecoder* decoder, WASMOpcode* out)
{
    ByteDecoder d = *decoder;

    u8 raw_opcode = 0;
    if (!byte_parse_u8(&d, &raw_opcode).found) {
        errorf("expected opcode");
        return kerror_unix(EINVAL);
    }

    switch (raw_opcode) {
#define M(name, ...) case WASMOpcode_##name:
    ENUMERATE_SINGLE_BYTE_WASM_OPCODES(M)
        break;
    default:
        errorf("unknown opcode 0x%0.2X", raw_opcode);
        return kerror_unix(EINVAL);
#undef M
    }

    *decoder = d;
    *out = raw_opcode;
    return kerror_none;
}

/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsubsection*.146
/// 3.3.10 Expressions
static KError wasm_expect_constinitexpr(ByteDecoder* decoder, WASMConstantInitializerExpression* out)
{
    ByteDecoder d = *decoder;

    WASMOpcode opcode = WASMOpcode_Invalid;
    TRY(wasm_expect_opcode(&d, &opcode));

    WASMConstantInitializerExpressionKind kind = WASMConstantInitializerExpression_Invalid;
    switch (opcode) {
    case WASMOpcode_i32_const:
    case WASMOpcode_i64_const:
        kind = (WASMConstantInitializerExpressionKind)opcode;
        break;
    default:
        errorf("opcode %lx cannot be used in a constant expression", opcode);
        return kerror_unix(EINVAL);
    }

    u64 initializer = 0;
    switch (kind) {
    case WASMConstantInitializerExpression_i32_const:
    case WASMConstantInitializerExpression_i64_const:
        if (!byte_parse_uleb128(&d, &initializer).found) {
            errorf("expected initial value for constant initializer");
            return kerror_unix(EINVAL);
        }
        if (kind == WASMConstantInitializerExpression_i32_const) {
            if (initializer > 0xFFFFFFFF) {
                errorf("initial value does not fit in a u32");
                return kerror_unix(EINVAL);
            }
        }
        break;
    case WASMConstantInitializerExpression_Invalid:
        UNREACHABLE();
        return kerror_unix(EINVAL);
    }

    u8 byte = 0;
    if (!byte_parse_u8(&d, &byte).found) {
        errorf("expected expression terminator, but found EOF");
        return kerror_unix(EINVAL);
    }
    if (byte != 0x0B) {
        errorf("expected expression terminator, but found 0x%.02X", byte);
        return kerror_unix(EINVAL);
    }

    *decoder = d;
    *out = (WASMConstantInitializerExpression){
        .constval = (WASMConstVal){
            .initializer = initializer,
        },
        .kind = kind,
    };
    return kerror_none;
}


/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.2.3.10
/// 2.3.10 Global Types
/// Global types classify global variables, which hold a value and can either be mutable or immutable.
static KError wasm_expect_global(ByteDecoder* decoder, WASMGlobal* out)
{
    ByteDecoder d = *decoder;

    WASMGlobalType type = { 0 };
    TRY(wasm_expect_globaltype(&d, &type));

    WASMConstantInitializerExpression init = { 0 };
    TRY(wasm_expect_constinitexpr(&d, &init));

    *decoder = d;
    *out = (WASMGlobal){
        .type = type,
        .init = init,
    };
    return kerror_none;
}


/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.2.5.10
/// 2.5.10 Exports
/// The exports component of a module defines a set of exports that become accessible to the host environment once
/// the module has been instantiated.
///
/// Each export is labeled by a unique name. Exportable definitions are functions, tables, memories, and globals,
/// which are referenced through a respective descriptor.
static KError wasm_expect_export(ByteDecoder* decoder, WASMExport* out)
{
    ByteDecoder d = *decoder;

    WASMName name = { 0 };
    TRY(wasm_expect_name(&d, &name));

    WASMExportDesc desc = { 0 };
    TRY(wasm_expect_export_desc(&d, &desc));

    *out = (WASMExport){
        .name = name,
        .desc = desc,
    };
    *decoder = d;
    return kerror_none;
}


/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.5.13
/// 5.5.13 Code Section
static KError wasm_expect_code(ByteDecoder* decoder, WASMCode* out)
{
    // FIXME: Use temporary for out.
    ByteDecoder d = *decoder;

    if (!byte_parse_uleb128(&d, &out->size).found) {
        errorf("expected function size");
        return kerror_unix(EINVAL);
    }
    TRY(wasm_expect_locals_vector(&d, &out->locals));
    TRY(wasm_expect_expression(&d, &out->expression));

    *decoder = d;
    return kerror_none;
}


/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.5.13
/// 5.5.13 Code Section
static KError wasm_expect_locals_vector(ByteDecoder* decoder, WASMLocalsVector* out)
{
    ByteDecoder d = *decoder;
    WASMLocalsVector result = { 0 };

    /// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.1.3
    /// 5.1.3 Vectors
    /// Vectors are encoded with their u32 length followed by the encoding of their element sequence.
    u64 vector_elements = 0;
    if (!byte_parse_uleb128(&d, &vector_elements).found) {
        errorf("locals array did not say how large the vector is");
        return kerror_unix(EINVAL);
    }
    if (vector_elements > static_size_left(&result)) {
        errorf("too many items in locals array (%lu, but max is %lu)", vector_elements, ARRAY_SIZE(result.items));
        return kerror_unix(EINVAL);
    }

    for (u64 i = 0; i < vector_elements; i++) {
        WASMLocals value = { 0 };
        TRY(wasm_expect_locals(&d, &value));
        TRY(static_push(&result, value));
    }

    *decoder = d;
    *out = result;
    return kerror_none;
}


/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.5.13
/// 5.5.13 Code Section (FIXME)
static KError wasm_expect_expression(ByteDecoder* decoder, WASMExpression* out)
{
    // FIXME: Use temporary for out.
    ByteDecoder d = *decoder;

    struct {
        u32 items[32];
        u32 count;
    } block_stack = { 0 };

    TRY(static_push(&block_stack, 0));
    u8 byte = 0;
    while (byte_peek_u8(&d, &byte).found) {
        VERIFY(byte_parse_u8(&d, &byte).found);

        if (byte == 0x0B) {
            if (block_stack.count == 0) goto eof;
            u32 offset = block_stack.items[block_stack.count - 1];
            WASMInstruction* i = &out->items[out->count - 1 - offset];
            VERIFY(i->opcode == WASMOpcode_block);
            i->b.u32 = offset;
            continue;
        }

        block_stack.items[block_stack.count - 1] += 1;
        WASMInstruction instruction = (WASMInstruction){ 0 };
        instruction.opcode = byte;
        switch (byte) {
        case WASMOpcode_unreachable:
        case WASMOpcode_return_:
        case WASMOpcode_drop:
        case WASMOpcode_i32_sub:
        case WASMOpcode_i32_add:
        case WASMOpcode_i32_and:
        case WASMOpcode_i32_eq:
        case WASMOpcode_i32_eqz:
            break;
        case WASMOpcode_block:
            TRY(static_push(&block_stack, 0));
            [[fallthrough]];
        case WASMOpcode_call:
        case WASMOpcode_i32_const:
        case WASMOpcode_i64_const:
        case WASMOpcode_f32_const:
        case WASMOpcode_f64_const:
        case WASMOpcode_local_get:
        case WASMOpcode_local_set:
        case WASMOpcode_global_get:
        case WASMOpcode_global_set:
        case WASMOpcode_br_if:
        case WASMOpcode_br:
            if (!byte_parse_uleb128(&d, &instruction.a.u64).found) {
                errorf("expected u32 value after %s instruction", wasm_opcode_string(instruction.opcode));
                return kerror_unix(EINVAL);
            }
            // FIXME: Check if u64 is out of range.
            break;
        case WASMOpcode_i32_load:
        case WASMOpcode_i32_store:
            if (!byte_parse_uleb128(&d, &instruction.a.u64).found) {
                errorf("expected u32 value after %s instruction", wasm_opcode_string(instruction.opcode));
                return kerror_unix(EINVAL);
            }
            if (!byte_parse_uleb128(&d, &instruction.b.u64).found) {
                errorf("expected second u32 value after %s instruction", wasm_opcode_string(instruction.opcode));
                return kerror_unix(EINVAL);
            }
            // FIXME: Check if u64 is out of range.
            break;
        default:
            errorf("don't know how to decode instruction %s(0x%.02X)", wasm_opcode_string((WASMOpcode)byte), byte);
            return kerror_unix(ENOTSUP);
        }

        TRY(static_push(out, instruction));
    }
eof:
    if (byte != 0x0B) {
        errorf("unexpected EOF");
        return kerror_unix(EINVAL);
    }

    *decoder = d;
    return kerror_none;
}

/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.5.13
/// 5.5.13 Code Section
static KError wasm_expect_locals(ByteDecoder* decoder, WASMLocals* out)
{
    ByteDecoder d = *decoder;

    u64 count = 0;
    if (!byte_parse_uleb128(&d, &count).found) {
        errorf("expected locals count");
        return kerror_unix(EINVAL);
    }

    if (count > wasm_locals_per_type_max) {
        errorf("too many locals (%ld)", count);
        return kerror_unix(EINVAL);
    }

    WASMValType valtype = 0;
    TRY(wasm_expect_valtype(&d, &valtype));

    *decoder = d;
    *out = (WASMLocals){
        .count = count,
        .type = valtype,
    };
    return kerror_none;
}


/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.5.4
/// 5.5.4 Type Section
/// The type section has the id 1. It decodes into a vector of function types that represent the types component of a module.
static KError wasm_expect_type_section(ByteDecoder* decoder, WASMTypeSection* out)
{
    ByteDecoder d = *decoder;
    WASMTypeSection result = { 0 };

    /// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.1.3
    /// 5.1.3 Vectors
    /// Vectors are encoded with their u32 length followed by the encoding of their element sequence.
    u64 vector_elements = 0;
    if (!byte_parse_uleb128(&d, &vector_elements).found) {
        errorf("type section did not say how large the vector is");
        return kerror_unix(EINVAL);
    }
    if (vector_elements > static_size_left(&result)) {
        errorf("too many types in type section");
        return kerror_unix(EINVAL);
    }

    for (u64 i = 0; i < vector_elements; i++) {
        WASMFuncType value = { 0 };
        TRY(wasm_expect_functype(&d, &value));
        TRY(static_push(&result, value));
    }

    *decoder = d;
    *out = result;
    return kerror_none;
}


/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.5.5
/// 5.5.5 Import Section
/// The import section has the id 2. It decodes into the list of imports of a module.
static KError wasm_expect_import_section(ByteDecoder* decoder, WASMImportSection* out)
{
    ByteDecoder d = *decoder;
    WASMImportSection result = { 0 };

    /// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.1.3
    /// 5.1.3 Vectors
    /// Vectors are encoded with their u32 length followed by the encoding of their element sequence.
    u64 vector_elements = 0;
    if (!byte_parse_uleb128(&d, &vector_elements).found) {
        errorf("import section did not say how large the vector is");
        return kerror_unix(EINVAL);
    }
    if (vector_elements > static_size_left(&result)) {
        errorf("too many items in import section");
        return kerror_unix(EINVAL);
    }

    for (u64 i = 0; i < vector_elements; i++) {
        WASMImport value = { 0 };
        TRY(wasm_expect_import(&d, &value));
        TRY(static_push(&result, value));
    }

    *decoder = d;
    *out = result;
    return kerror_none;
}


/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.5.5
/// 5.5.5 Import Section
/// The import section has the id 2. It decodes into the list of imports of a module.
static KError wasm_expect_import(ByteDecoder* decoder, WASMImport* out)
{
    ByteDecoder d = *decoder;

    WASMName mod = { 0 };
    TRY(wasm_expect_name(&d, &mod));

    WASMName name = { 0 };
    TRY(wasm_expect_name(&d, &name));

    WASMExternType externtype = { 0 };
    TRY(wasm_expect_externtype(&d, &externtype));

    *decoder = d;
    *out = (WASMImport){
        .mod = mod,
        .name = name,
        .externtype = externtype,
    };
    return kerror_none;
}


/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.3.14
/// 5.3.14 External Types
/// External types are encoded by a distiguishing byte followed by an encoding of the respective form of type.
static KError wasm_expect_externtype(ByteDecoder* decoder, WASMExternType* out)
{
    ByteDecoder d = *decoder;
    WASMExternType result = { 0 };

    u8 byte = 0;
    if (!byte_parse_u8(&d, &byte).found) {
        errorf("expected externtype discriminator");
        return kerror_unix(EINVAL);
    }

    switch (byte) {
    case WASMExternTypeTag_typeidx:
        result.tag = WASMExternTypeTag_typeidx;
        TRY(wasm_expect_typeidx(&d, &result.typeidx));
        break;
    case WASMExternTypeTag_tableidx:
        result.tag = WASMExternTypeTag_tableidx;
        TRY(wasm_expect_tableidx(&d, &result.tableidx));
        break;
    case WASMExternTypeTag_memidx:
        result.tag = WASMExternTypeTag_memidx;
        TRY(wasm_expect_memidx(&d, &result.memidx));
        break;
    case WASMExternTypeTag_globalidx:
        result.tag = WASMExternTypeTag_globalidx;
        TRY(wasm_expect_globalidx(&d, &result.globalidx));
        break;
    case WASMExternTypeTag_tagtype:
        result.tag = WASMExternTypeTag_tagtype;
        TRY(wasm_expect_tagidx(&d, &result.tagidx));
        break;
    default:
        errorf("unknown externtype discriminator 0x%.02X", byte);
        return kerror_unix(EINVAL);
    }

    *decoder = d;
    *out = result;
    return kerror_none;
}


/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.5.6
/// 5.5.6 Function Section
/// functions in the funcs component of a module. The locals and body fields of the respective functions are encoded
/// separately in the code section.
static KError wasm_expect_function_section(ByteDecoder* decoder, WASMFunctionSection* out)
{
    ByteDecoder d = *decoder;
    WASMFunctionSection result = { 0 };

    /// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.1.3
    /// 5.1.3 Vectors
    /// Vectors are encoded with their u32 length followed by the encoding of their element sequence.
    u64 vector_elements = 0;
    if (!byte_parse_uleb128(&d, &vector_elements).found) {
        errorf("function section did not say how large the vector is");
        return kerror_unix(EINVAL);
    }
    if (vector_elements > static_size_left(&result)) {
        errorf("too many items in function section");
        return kerror_unix(EINVAL);
    }

    for (u64 i = 0; i < vector_elements; i++) {
        WASMTypeIdx value = { 0 };
        TRY(wasm_expect_typeidx(&d, &value));
        TRY(static_push(&result, value));
    }

    *decoder = d;
    *out = result;
    return kerror_none;
}


/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.5.8
/// 5.5.8 Memory Section
/// The memory section has the id 5. It decodes into a vector of memories that represent the mems component of a
/// module.
/// NOTE: In the current version of WebAssembly, at most one memory may be defined or imported in a single module,
/// and all constructs implicitly reference this memory 0. This restriction may be lifted in future versions.
static KError wasm_expect_memory_section(ByteDecoder* decoder, WASMMemorySection* out)
{
    ByteDecoder d = *decoder;
    WASMMemorySection result = { 0 };

    /// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.1.3
    /// 5.1.3 Vectors
    /// Vectors are encoded with their u32 length followed by the encoding of their element sequence.
    u64 vector_elements = 0;
    if (!byte_parse_uleb128(&d, &vector_elements).found) {
        errorf("function section did not say how large the vector is");
        return kerror_unix(EINVAL);
    }
    if (vector_elements > static_size_left(&result)) {
        errorf("too many items in function section");
        return kerror_unix(EINVAL);
    }

    for (u64 i = 0; i < vector_elements; i++) {
        WASMMemType value = { 0 };
        TRY(wasm_expect_memtype(&d, &value));
        TRY(static_push(&result, value));
    }

    *decoder = d;
    *out = result;
    return kerror_none;
}


/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.5.9
/// 5.5.9 Global Section
/// The global section has the id 6. It decodes into a vector of globals that represent the globals component of a module.
static KError wasm_expect_global_section(ByteDecoder* decoder, WASMGlobalSection* out)
{
    ByteDecoder d = *decoder;
    WASMGlobalSection result = { 0 };

    /// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.1.3
    /// 5.1.3 Vectors
    /// Vectors are encoded with their u32 length followed by the encoding of their element sequence.
    u64 vector_elements = 0;
    if (!byte_parse_uleb128(&d, &vector_elements).found) {
        errorf("global section did not say how large the vector is");
        return kerror_unix(EINVAL);
    }
    if (vector_elements > static_size_left(&result)) {
        errorf("too many items in global section");
        return kerror_unix(EINVAL);
    }

    for (u64 i = 0; i < vector_elements; i++) {
        WASMGlobal value = { 0 };
        TRY(wasm_expect_global(&d, &value));
        TRY(static_push(&result, value));
    }

    *decoder = d;
    *out = result;
    return kerror_none;
}


/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.2.5.10
/// 2.5.10 Exports section
/// The exports component of a module defines a set of exports that become accessible to the host environment once
/// the module has been instantiated.
static KError wasm_expect_export_section(ByteDecoder* decoder, WASMExportSection* out)
{
    ByteDecoder d = *decoder;
    WASMExportSection result = { 0 };

    /// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.1.3
    /// 5.1.3 Vectors
    /// Vectors are encoded with their u32 length followed by the encoding of their element sequence.
    u64 vector_elements = 0;
    if (!byte_parse_uleb128(&d, &vector_elements).found) {
        errorf("function section did not say how large the vector is");
        return kerror_unix(EINVAL);
    }
    if (vector_elements > static_size_left(&result)) {
        errorf("too many items in function section");
        return kerror_unix(EINVAL);
    }

    for (u64 i = 0; i < vector_elements; i++) {
        WASMExport value = { 0 };
        TRY(wasm_expect_export(&d, &value));
        TRY(static_push(&result, value));
    }

    *decoder = d;
    *out = result;
    return kerror_none;
}


/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.5.13
/// 5.5.13 Code Section
static KError wasm_expect_code_section(ByteDecoder* decoder, WASMCodeSection* out)
{
    // FIXME: Use local for out.
    ByteDecoder d = *decoder;

    /// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.1.3
    /// 5.1.3 Vectors
    /// Vectors are encoded with their u32 length followed by the encoding of their element sequence.
    u64 vector_elements = 0;
    if (!byte_parse_uleb128(&d, &vector_elements).found) {
        errorf("code section did not say how large the vector is");
        return kerror_unix(EINVAL);
    }
    if (vector_elements > static_size_left(out)) {
        errorf("too many items in code section");
        return kerror_unix(EINVAL);
    }

    for (u64 i = 0; i < vector_elements; i++) {
        WASMCode value = { 0 };
        TRY(wasm_expect_code(&d, &value));
        TRY(static_push(out, value));
    }

    *decoder = d;
    return kerror_none;
}

/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.5.4
/// 5.5.4 Data Section
static KError wasm_expect_data_section(ByteDecoder* decoder, WASMMemory* memory)
{
    ByteDecoder d = *decoder;

    /// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.1.3
    /// 5.1.3 Vectors
    /// Vectors are encoded with their u32 length followed by the encoding of their element sequence.
    u64 vector_elements = 0;
    if (!byte_parse_uleb128(&d, &vector_elements).found) {
        errorf("data section did not say how large the vector is");
        return kerror_unix(EINVAL);
    }

    for (u64 i = 0; i < vector_elements; i++) {
        Bytes apply = { 0 };
        u64 apply_at_index = 0;
        TRY(wasm_expect_data_segment(&d, &apply, &apply_at_index));
        if (apply_at_index + apply.count >= ARRAY_SIZE(memory->items)) {
            errorf("applying data section would result in out of bounds write");
            return kerror_unix(EINVAL);
        }
        memcpy(&memory->items[apply_at_index], apply.items, apply.count);
    }

    *decoder = d;
    return kerror_none;
}


/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.5.4
/// 5.5.4 Data Section
static KError wasm_expect_data_segment(ByteDecoder* decoder, Bytes* apply, u64* apply_at_index)
{
    ByteDecoder d = *decoder;

    u64 data_type = 0;
    if (!byte_parse_uleb128(&d, &data_type).found) {
        errorf("expected binary data type");
        return kerror_unix(EINVAL);
    }

    switch (data_type) {
    case 0: // raw data
        break;
    case 1:
    case 2:
    default:
        errorf("do not know how to parse data type %lu", data_type);
        return kerror_unix(EINVAL);
    }

    WASMConstantInitializerExpression init = { 0 };
    TRY(wasm_expect_constinitexpr(&d, &init));

    VERIFY(init.kind == WASMConstantInitializerExpression_i32_const);
    *apply_at_index = init.constval.initializer;

    u64 count = 0;
    if (!byte_parse_uleb128(&d, &count).found) {
        errorf("expected binary payload size");
        return kerror_unix(EINVAL);
    }

    if (!byte_parse_bytes(&d, count, apply).found) {
        errorf("expected %ld more bytes", count);
        return kerror_unix(EINVAL);
    }

    *decoder = d;
    return kerror_none;
}

C_API c_string wasm_export_desc_kind_string(WASMExportDescKind kind)
{
    switch (kind) {
    case WASMExportDescKind_Func:   return "func";
    case WASMExportDescKind_Table:  return "table";
    case WASMExportDescKind_Memory: return "memory";
    case WASMExportDescKind_Global: return "global";
    }
}

C_API c_string wasm_valtype_string(WASMValType type)
{
    switch (type) {
    case WASMValType_i32: return "i32";
    case WASMValType_i64: return "i64";
    case WASMValType_f32: return "f32";
    case WASMValType_f64: return "f64";

    case WASMValType_v128: return "v128";

    case WASMValType_FuncRef: return "funcref";
    case WASMValType_ExternRef: return "externref";
    }
}

C_API c_string wasm_externtype_tag_string(WASMExternTypeTag tag)
{
    switch (tag) {
    case WASMExternTypeTag_typeidx: return "typeidx";
    case WASMExternTypeTag_tableidx: return "tableidx";
    case WASMExternTypeTag_memidx: return "memidx";
    case WASMExternTypeTag_globalidx: return "globalidx";
    case WASMExternTypeTag_tagtype: return "tagtype";
    }
}

C_API c_string wasm_opcode_string(WASMOpcode opcode)
{
    switch (opcode) {
#define M(name, ...) case WASMOpcode_##name: return #name;
    ENUMERATE_WASM_OPCODES(M);
    case WASMOpcode_Invalid: return "invalid";
#undef M
    default: return "unknown";
    }
}

static void wasm_vm_disassemble(WASMVirtualMachine* vm, WASMInstruction instruction);

static i64 wasm_mod_function_import_count(WASMModule const* mod)
{
    i64 count = 0;
    for (u64 i = 0; i < mod->import_section.count; i++) {
        WASMImport import = mod->import_section.items[i];
        if (import.externtype.tag == WASMExternTypeTag_typeidx)
            count += 1;
    }
    return count;
}

C_API KError wasm_vm_call(WASMVirtualMachine* vm, WASMFunctionID function)
{
    WASMModule const* mod = vm->module;
    i64 code_index = ((i64)function.index) - wasm_mod_function_import_count(mod);
    if (code_index < 0) {
        u64 native_index = function.index;
        if (native_index >= ARRAY_SIZE(mod->native_functions)) {
            errorf("function is out of range");
            return kerror_unix(EINVAL);
        }
        WASMNativeFunction func = mod->native_functions[native_index];
        if (!func.callback) {
            if (native_index >= ARRAY_SIZE(mod->import_section.items)) {
                errorf("function import is out of range");
                return kerror_unix(EINVAL);
            }
            WASMImport const* import = &mod->import_section.items[native_index];

            errorf("function is not bound: '%.*s':'%.*s'",
                    import->mod.count,
                    import->mod.items,
                    import->name.count,
                    import->name.items
            );
            return kerror_unix(EINVAL);
        }
        
        u64 stack_index = vm->stack.count;
        u64 expected_index = stack_index - func.inputs + func.outputs;
        KError result = func.callback(vm);
        if (!result.ok) return result;
        if (vm->stack.count != expected_index) {
            errorf("function '%s.%s' did not fulfill its signature", func.mod, func.name);
            return kerror_unix(EINVAL);
        }
        return kerror_none;
    }

    if (code_index >= mod->code_section.count) {
        errorf("function is out of range");
        return kerror_unix(EINVAL);
    }
    WASMCode const* code = &mod->code_section.items[code_index];

    u32 i32_locals_count = 0;
    for (u32 i = 0; i < code->locals.count; i++) {
        if (code->locals.items[i].count != 0) {
            VERIFY(code->locals.items[i].type == WASMValType_i32);
            i32_locals_count = code->locals.items[i].count;
        }
    }

    if (i32_locals_count >= static_size_left(&vm->locals_stack))
        return kerror_unix(ENOMEM);
    memzero(vm->locals_stack.items + vm->locals_stack.count, i32_locals_count * sizeof(vm->locals_stack.items[0]));
    u32* locals = vm->locals_stack.items + vm->locals_stack.count;
    vm->locals_stack.count += i32_locals_count;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winitializer-overrides"
    void* instruction[0x100] = {
        [0 ... 0xFF] = &&unimplemented,
#define M(name, ...) [WASMOpcode_##name] = &&name,
        ENUMERATE_SINGLE_BYTE_WASM_OPCODES(M)
#undef M
    };
    void* extended_instruction[0x100] = {
        [0 ... 0xFF] = &&unimplemented,
#define M(name, ...) [WASMOpcode_##name & 0xFFFF] = &&name,
        ENUMERATE_MULTI_BYTE_WASM_OPCODES(M)
#undef M
    };
#pragma clang diagnostic pop

    u64 ip = 0;
    WASMInstruction current = { 0 };

#define NEXT()                                              \
    if (ip >= code->expression.count) goto return_;         \
    current = code->expression.items[ip++];                 \
    wasm_vm_disassemble(vm, current);                       \
    if (current.opcode & 0xFF00000000000000)                \
        goto* extended_instruction[current.opcode & 0xFF];  \
    goto* instruction[current.opcode];

    NEXT();
unimplemented:
    errorf("unimplemented opcode %s", wasm_opcode_string(current.opcode));
    return kerror_unix(ENOTSUP);
unreachable:
    UNREACHABLE();
nop:
    NEXT();
block:
    warnf("don't know what to do with block %d:%d", current.a.u32, current.b.u32);
    NEXT();
loop:
    goto unimplemented;
if_:
    goto unimplemented;
br:
    goto unimplemented;
br_if:
    goto unimplemented;
br_table:
    goto unimplemented;
return_:
    vm->locals_stack.count -= i32_locals_count;
    return kerror_none;
call:
    TRY(wasm_vm_call(vm, (WASMFunctionID){current.a.i32}));
    NEXT();
call_indirect:
    goto unimplemented;
drop:
    TRY(wasm_vm_pop_i32(vm, nullptr));
    NEXT();
select:
    goto unimplemented;
select_typed:
    goto unimplemented;
local_get:
    if (current.a.u32 >= i32_locals_count) {
        errorf("trying to index local out of range: %u >= %u", current.a.u32, i32_locals_count);
        return kerror_unix(EINVAL);
    }
    TRY(wasm_vm_push(vm, locals[current.a.u32]));
    NEXT();
local_set:
    if (current.a.u32 >= i32_locals_count) {
        errorf("trying to index local out of range: %u >= %u", current.a.u32, i32_locals_count);
        return kerror_unix(EINVAL);
    }
    TRY(wasm_vm_pop(vm, &locals[current.a.u32]));
    NEXT();
local_tee:
    goto unimplemented;
global_get:
    if (current.a.u32 >= ARRAY_SIZE(vm->globals.items)) {
        errorf("trying to index global out of range");
        return kerror_unix(EINVAL);
    }
    TRY(wasm_vm_push(vm, vm->globals.items[current.a.u32]));
    NEXT();
global_set:
    if (current.a.u32 >= ARRAY_SIZE(vm->globals.items)) {
        errorf("trying to index global out of range");
        return kerror_unix(EINVAL);
    }
    TRY(wasm_vm_pop(vm, &vm->globals.items[current.a.u32]));
    NEXT();
table_get:
    goto unimplemented;
table_set:
    goto unimplemented;
i32_load:
    switch (current.a.u32) {
    case 2:
        if (current.b.u32 + 4 >= ARRAY_SIZE(vm->memory.items)) {
            errorf("trying to index memory out of bounds");
            return kerror_unix(EINVAL);
        }
        {
            u32 value = 0;
            memcpy(&value, &vm->memory.items[current.b.u32], sizeof(value));
            TRY(wasm_vm_push(vm, value));
        }
        NEXT();
    default:
        UNIMPLEMENTED();
    }
    UNREACHABLE();
    goto unimplemented;
i64_load:
    goto unimplemented;
f32_load:
    goto unimplemented;
f64_load:
    goto unimplemented;
i32_load8_s:
    goto unimplemented;
i32_load8_u:
    goto unimplemented;
i32_load16_s:
    goto unimplemented;
i32_load16_u:
    goto unimplemented;
i64_load8_s:
    goto unimplemented;
i64_load8_u:
    goto unimplemented;
i64_load16_s:
    goto unimplemented;
i64_load16_u:
    goto unimplemented;
i64_load32_s:
    goto unimplemented;
i64_load32_u:
    goto unimplemented;
i32_store:
    switch (current.a.u32) {
    case 2:
        if (current.b.u32 + 4 >= ARRAY_SIZE(vm->memory.items)) {
            errorf("trying to index memory out of bounds");
            return kerror_unix(EINVAL);
        }
        {
            u32 value = 0;
            TRY(wasm_vm_pop(vm, &value));
            memcpy(&vm->memory.items[current.b.u32], &value, sizeof(value));
        }
        NEXT();
    default:
        UNIMPLEMENTED();
    }
    UNREACHABLE();
i64_store:
    goto unimplemented;
f32_store:
    goto unimplemented;
f64_store:
    goto unimplemented;
i32_store8:
    goto unimplemented;
i32_store16:
    goto unimplemented;
i64_store8:
    goto unimplemented;
i64_store16:
    goto unimplemented;
i64_store32:
    goto unimplemented;
memory_size:
    goto unimplemented;
memory_grow:
    goto unimplemented;
i32_const:
    TRY(wasm_vm_push_i32(vm, current.a.i32));
    NEXT();
i64_const:
    goto unimplemented;
f32_const:
    TRY(wasm_vm_push_f32(vm, current.a.f32));
    NEXT();
f64_const:
    goto unimplemented;
i32_eqz:
    {
        u32 a = 0;
        TRY(wasm_vm_pop(vm, &a));
        TRY(wasm_vm_push(vm, a == 0));
    }
    NEXT();
i32_eq:
    {
        u32 a = 0;
        u32 b = 0;
        TRY(wasm_vm_pop(vm, &a));
        TRY(wasm_vm_pop(vm, &b));
        TRY(wasm_vm_push(vm, a == b));
    }
    NEXT();
i32_ne:
    {
        u32 a = 0;
        u32 b = 0;
        TRY(wasm_vm_pop(vm, &a));
        TRY(wasm_vm_pop(vm, &b));
        TRY(wasm_vm_push(vm, a != b));
    }
    NEXT();
i32_lts:
    goto unimplemented;
i32_ltu:
    goto unimplemented;
i32_gts:
    goto unimplemented;
i32_gtu:
    goto unimplemented;
i32_les:
    goto unimplemented;
i32_leu:
    goto unimplemented;
i32_ges:
    goto unimplemented;
i32_geu:
    goto unimplemented;
i64_eqz:
    goto unimplemented;
i64_eq:
    goto unimplemented;
i64_ne:
    goto unimplemented;
i64_lts:
    goto unimplemented;
i64_ltu:
    goto unimplemented;
i64_gts:
    goto unimplemented;
i64_gtu:
    goto unimplemented;
i64_les:
    goto unimplemented;
i64_leu:
    goto unimplemented;
i64_ges:
    goto unimplemented;
i64_geu:
    goto unimplemented;
f32_eq:
    goto unimplemented;
f32_ne:
    goto unimplemented;
f32_lt:
    goto unimplemented;
f32_gt:
    goto unimplemented;
f32_le:
    goto unimplemented;
f32_ge:
    goto unimplemented;
f64_eq:
    goto unimplemented;
f64_ne:
    goto unimplemented;
f64_lt:
    goto unimplemented;
f64_gt:
    goto unimplemented;
f64_le:
    goto unimplemented;
f64_ge:
    goto unimplemented;
i32_clz:
    goto unimplemented;
i32_ctz:
    goto unimplemented;
i32_popcnt:
    goto unimplemented;
i32_add:
    {
        u32 a = 0;
        u32 b = 0;
        TRY(wasm_vm_pop(vm, &a));
        TRY(wasm_vm_pop(vm, &b));
        TRY(wasm_vm_push(vm, a + b));
    }
    NEXT();
i32_sub:
    {
        u32 a = 0;
        u32 b = 0;
        TRY(wasm_vm_pop(vm, &a));
        TRY(wasm_vm_pop(vm, &b));
        TRY(wasm_vm_push(vm, a - b));
    }
    NEXT();
i32_mul:
    {
        u32 a = 0;
        u32 b = 0;
        TRY(wasm_vm_pop(vm, &a));
        TRY(wasm_vm_pop(vm, &b));
        TRY(wasm_vm_push(vm, a * b));
    }
    NEXT();
i32_divs:
    {
        i32 a = 0;
        i32 b = 0;
        TRY(wasm_vm_pop(vm, &a));
        TRY(wasm_vm_pop(vm, &b));
        TRY(wasm_vm_push(vm, a * b));
    }
    NEXT();
i32_divu:
    goto unimplemented;
i32_rems:
    goto unimplemented;
i32_remu:
    goto unimplemented;
i32_and:
    {
        u32 a = 0;
        u32 b = 0;
        TRY(wasm_vm_pop(vm, &a));
        TRY(wasm_vm_pop(vm, &b));
        TRY(wasm_vm_push(vm, a & b));
    }
    NEXT();
i32_or:
    {
        u32 a = 0;
        u32 b = 0;
        TRY(wasm_vm_pop(vm, &a));
        TRY(wasm_vm_pop(vm, &b));
        TRY(wasm_vm_push(vm, a | b));
    }
    NEXT();
i32_xor:
    {
        u32 a = 0;
        u32 b = 0;
        TRY(wasm_vm_pop(vm, &a));
        TRY(wasm_vm_pop(vm, &b));
        TRY(wasm_vm_push(vm, a ^ b));
    }
    NEXT();
i32_shl:
    {
        u32 a = 0;
        u32 b = 0;
        TRY(wasm_vm_pop(vm, &a));
        TRY(wasm_vm_pop(vm, &b));
        TRY(wasm_vm_push(vm, a << b));
    }
    NEXT();
i32_shrs:
    {
        i32 a = 0;
        i32 b = 0;
        TRY(wasm_vm_pop(vm, &a));
        TRY(wasm_vm_pop(vm, &b));
        TRY(wasm_vm_push(vm, a >> b));
    }
    NEXT();
i32_shru:
    {
        u32 a = 0;
        u32 b = 0;
        TRY(wasm_vm_pop(vm, &a));
        TRY(wasm_vm_pop(vm, &b));
        TRY(wasm_vm_push(vm, a >> b));
    }
    NEXT();
i32_rotl:
    goto unimplemented;
i32_rotr:
    goto unimplemented;
i64_clz:
    goto unimplemented;
i64_ctz:
    goto unimplemented;
i64_popcnt:
    goto unimplemented;
i64_add:
    goto unimplemented;
i64_sub:
    goto unimplemented;
i64_mul:
    goto unimplemented;
i64_divs:
    goto unimplemented;
i64_divu:
    goto unimplemented;
i64_rems:
    goto unimplemented;
i64_remu:
    goto unimplemented;
i64_and:
    goto unimplemented;
i64_or:
    goto unimplemented;
i64_xor:
    goto unimplemented;
i64_shl:
    goto unimplemented;
i64_shrs:
    goto unimplemented;
i64_shru:
    goto unimplemented;
i64_rotl:
    goto unimplemented;
i64_rotr:
    goto unimplemented;
f32_abs:
    goto unimplemented;
f32_neg:
    goto unimplemented;
f32_ceil:
    goto unimplemented;
f32_floor:
    goto unimplemented;
f32_trunc:
    goto unimplemented;
f32_nearest:
    goto unimplemented;
f32_sqrt:
    goto unimplemented;
f32_add:
    goto unimplemented;
f32_sub:
    goto unimplemented;
f32_mul:
    goto unimplemented;
f32_div:
    goto unimplemented;
f32_min:
    goto unimplemented;
f32_max:
    goto unimplemented;
f32_copysign:
    goto unimplemented;
f64_abs:
    goto unimplemented;
f64_neg:
    goto unimplemented;
f64_ceil:
    goto unimplemented;
f64_floor:
    goto unimplemented;
f64_trunc:
    goto unimplemented;
f64_nearest:
    goto unimplemented;
f64_sqrt:
    goto unimplemented;
f64_add:
    goto unimplemented;
f64_sub:
    goto unimplemented;
f64_mul:
    goto unimplemented;
f64_div:
    goto unimplemented;
f64_min:
    goto unimplemented;
f64_max:
    goto unimplemented;
f64_copysign:
    goto unimplemented;
i32_wrap_i64:
    goto unimplemented;
i32_trunc_sf32:
    goto unimplemented;
i32_trunc_uf32:
    goto unimplemented;
i32_trunc_sf64:
    goto unimplemented;
i32_trunc_uf64:
    goto unimplemented;
i64_extend_si32:
    goto unimplemented;
i64_extend_ui32:
    goto unimplemented;
i64_trunc_sf32:
    goto unimplemented;
i64_trunc_uf32:
    goto unimplemented;
i64_trunc_sf64:
    goto unimplemented;
i64_trunc_uf64:
    goto unimplemented;
f32_convert_si32:
    goto unimplemented;
f32_convert_ui32:
    goto unimplemented;
f32_convert_si64:
    goto unimplemented;
f32_convert_ui64:
    goto unimplemented;
f32_demote_f64:
    goto unimplemented;
f64_convert_si32:
    goto unimplemented;
f64_convert_ui32:
    goto unimplemented;
f64_convert_si64:
    goto unimplemented;
f64_convert_ui64:
    goto unimplemented;
f64_promote_f32:
    goto unimplemented;
i32_reinterpret_f32:
    goto unimplemented;
i64_reinterpret_f64:
    goto unimplemented;
f32_reinterpret_i32:
    goto unimplemented;
f64_reinterpret_i64:
    goto unimplemented;
i32_extend8_s:
    goto unimplemented;
i32_extend16_s:
    goto unimplemented;
i64_extend8_s:
    goto unimplemented;
i64_extend16_s:
    goto unimplemented;
i64_extend32_s:
    goto unimplemented;
ref_null:
    goto unimplemented;
ref_is_null:
    goto unimplemented;
ref_func:
    goto unimplemented;

i32_trunc_sat_f32_s:
    goto unimplemented;
i32_trunc_sat_f32_u:
    goto unimplemented;
i32_trunc_sat_f64_s:
    goto unimplemented;
i32_trunc_sat_f64_u:
    goto unimplemented;
i64_trunc_sat_f32_s:
    goto unimplemented;
i64_trunc_sat_f32_u:
    goto unimplemented;
i64_trunc_sat_f64_s:
    goto unimplemented;
i64_trunc_sat_f64_u:
    goto unimplemented;
memory_init:
    goto unimplemented;
data_drop:
    goto unimplemented;
memory_copy:
    goto unimplemented;
memory_fill:
    goto unimplemented;
table_init:
    goto unimplemented;
elem_drop:
    goto unimplemented;
table_copy:
    goto unimplemented;
table_grow:
    goto unimplemented;
table_size:
    goto unimplemented;
table_fill:
    goto unimplemented;
structured_else:
    goto unimplemented;
structured_end:
    goto unimplemented;
v128_load:
    goto unimplemented;
v128_load8x8_s:
    goto unimplemented;
v128_load8x8_u:
    goto unimplemented;
v128_load16x4_s:
    goto unimplemented;
v128_load16x4_u:
    goto unimplemented;
v128_load32x2_s:
    goto unimplemented;
v128_load32x2_u:
    goto unimplemented;
v128_load8_splat:
    goto unimplemented;
v128_load16_splat:
    goto unimplemented;
v128_load32_splat:
    goto unimplemented;
v128_load64_splat:
    goto unimplemented;
v128_store:
    goto unimplemented;
v128_const:
    goto unimplemented;
i8x16_shuffle:
    goto unimplemented;
i8x16_swizzle:
    goto unimplemented;
i8x16_splat:
    goto unimplemented;
i16x8_splat:
    goto unimplemented;
i32x4_splat:
    goto unimplemented;
i64x2_splat:
    goto unimplemented;
f32x4_splat:
    goto unimplemented;
f64x2_splat:
    goto unimplemented;
i8x16_extract_lane_s:
    goto unimplemented;
i8x16_extract_lane_u:
    goto unimplemented;
i8x16_replace_lane:
    goto unimplemented;
i16x8_extract_lane_s:
    goto unimplemented;
i16x8_extract_lane_u:
    goto unimplemented;
i16x8_replace_lane:
    goto unimplemented;
i32x4_extract_lane:
    goto unimplemented;
i32x4_replace_lane:
    goto unimplemented;
i64x2_extract_lane:
    goto unimplemented;
i64x2_replace_lane:
    goto unimplemented;
f32x4_extract_lane:
    goto unimplemented;
f32x4_replace_lane:
    goto unimplemented;
f64x2_extract_lane:
    goto unimplemented;
f64x2_replace_lane:
    goto unimplemented;
i8x16_eq:
    goto unimplemented;
i8x16_ne:
    goto unimplemented;
i8x16_lt_s:
    goto unimplemented;
i8x16_lt_u:
    goto unimplemented;
i8x16_gt_s:
    goto unimplemented;
i8x16_gt_u:
    goto unimplemented;
i8x16_le_s:
    goto unimplemented;
i8x16_le_u:
    goto unimplemented;
i8x16_ge_s:
    goto unimplemented;
i8x16_ge_u:
    goto unimplemented;
i16x8_eq:
    goto unimplemented;
i16x8_ne:
    goto unimplemented;
i16x8_lt_s:
    goto unimplemented;
i16x8_lt_u:
    goto unimplemented;
i16x8_gt_s:
    goto unimplemented;
i16x8_gt_u:
    goto unimplemented;
i16x8_le_s:
    goto unimplemented;
i16x8_le_u:
    goto unimplemented;
i16x8_ge_s:
    goto unimplemented;
i16x8_ge_u:
    goto unimplemented;
i32x4_eq:
    goto unimplemented;
i32x4_ne:
    goto unimplemented;
i32x4_lt_s:
    goto unimplemented;
i32x4_lt_u:
    goto unimplemented;
i32x4_gt_s:
    goto unimplemented;
i32x4_gt_u:
    goto unimplemented;
i32x4_le_s:
    goto unimplemented;
i32x4_le_u:
    goto unimplemented;
i32x4_ge_s:
    goto unimplemented;
i32x4_ge_u:
    goto unimplemented;
f32x4_eq:
    goto unimplemented;
f32x4_ne:
    goto unimplemented;
f32x4_lt:
    goto unimplemented;
f32x4_gt:
    goto unimplemented;
f32x4_le:
    goto unimplemented;
f32x4_ge:
    goto unimplemented;
f64x2_eq:
    goto unimplemented;
f64x2_ne:
    goto unimplemented;
f64x2_lt:
    goto unimplemented;
f64x2_gt:
    goto unimplemented;
f64x2_le:
    goto unimplemented;
f64x2_ge:
    goto unimplemented;
v128_not:
    goto unimplemented;
v128_and:
    goto unimplemented;
v128_andnot:
    goto unimplemented;
v128_or:
    goto unimplemented;
v128_xor:
    goto unimplemented;
v128_bitselect:
    goto unimplemented;
v128_any_true:
    goto unimplemented;
v128_load8_lane:
    goto unimplemented;
v128_load16_lane:
    goto unimplemented;
v128_load32_lane:
    goto unimplemented;
v128_load64_lane:
    goto unimplemented;
v128_store8_lane:
    goto unimplemented;
v128_store16_lane:
    goto unimplemented;
v128_store32_lane:
    goto unimplemented;
v128_store64_lane:
    goto unimplemented;
v128_load32_zero:
    goto unimplemented;
v128_load64_zero:
    goto unimplemented;
f32x4_demote_f64x2_zero:
    goto unimplemented;
f64x2_promote_low_f32x4:
    goto unimplemented;
i8x16_abs:
    goto unimplemented;
i8x16_neg:
    goto unimplemented;
i8x16_popcnt:
    goto unimplemented;
i8x16_all_true:
    goto unimplemented;
i8x16_bitmask:
    goto unimplemented;
i8x16_narrow_i16x8_s:
    goto unimplemented;
i8x16_narrow_i16x8_u:
    goto unimplemented;
f32x4_ceil:
    goto unimplemented;
f32x4_floor:
    goto unimplemented;
f32x4_trunc:
    goto unimplemented;
f32x4_nearest:
    goto unimplemented;
i8x16_shl:
    goto unimplemented;
i8x16_shr_s:
    goto unimplemented;
i8x16_shr_u:
    goto unimplemented;
i8x16_add:
    goto unimplemented;
i8x16_add_sat_s:
    goto unimplemented;
i8x16_add_sat_u:
    goto unimplemented;
i8x16_sub:
    goto unimplemented;
i8x16_sub_sat_s:
    goto unimplemented;
i8x16_sub_sat_u:
    goto unimplemented;
f64x2_ceil:
    goto unimplemented;
f64x2_floor:
    goto unimplemented;
i8x16_min_s:
    goto unimplemented;
i8x16_min_u:
    goto unimplemented;
i8x16_max_s:
    goto unimplemented;
i8x16_max_u:
    goto unimplemented;
f64x2_trunc:
    goto unimplemented;
i8x16_avgr_u:
    goto unimplemented;
i16x8_extadd_pairwise_i8x16_s:
    goto unimplemented;
i16x8_extadd_pairwise_i8x16_u:
    goto unimplemented;
i32x4_extadd_pairwise_i16x8_s:
    goto unimplemented;
i32x4_extadd_pairwise_i16x8_u:
    goto unimplemented;
i16x8_abs:
    goto unimplemented;
i16x8_neg:
    goto unimplemented;
i16x8_q15mulr_sat_s:
    goto unimplemented;
i16x8_all_true:
    goto unimplemented;
i16x8_bitmask:
    goto unimplemented;
i16x8_narrow_i32x4_s:
    goto unimplemented;
i16x8_narrow_i32x4_u:
    goto unimplemented;
i16x8_extend_low_i8x16_s:
    goto unimplemented;
i16x8_extend_high_i8x16_s:
    goto unimplemented;
i16x8_extend_low_i8x16_u:
    goto unimplemented;
i16x8_extend_high_i8x16_u:
    goto unimplemented;
i16x8_shl:
    goto unimplemented;
i16x8_shr_s:
    goto unimplemented;
i16x8_shr_u:
    goto unimplemented;
i16x8_add:
    goto unimplemented;
i16x8_add_sat_s:
    goto unimplemented;
i16x8_add_sat_u:
    goto unimplemented;
i16x8_sub:
    goto unimplemented;
i16x8_sub_sat_s:
    goto unimplemented;
i16x8_sub_sat_u:
    goto unimplemented;
f64x2_nearest:
    goto unimplemented;
i16x8_mul:
    goto unimplemented;
i16x8_min_s:
    goto unimplemented;
i16x8_min_u:
    goto unimplemented;
i16x8_max_s:
    goto unimplemented;
i16x8_max_u:
    goto unimplemented;
i16x8_avgr_u:
    goto unimplemented;
i16x8_extmul_low_i8x16_s:
    goto unimplemented;
i16x8_extmul_high_i8x16_s:
    goto unimplemented;
i16x8_extmul_low_i8x16_u:
    goto unimplemented;
i16x8_extmul_high_i8x16_u:
    goto unimplemented;
i32x4_abs:
    goto unimplemented;
i32x4_neg:
    goto unimplemented;
i32x4_all_true:
    goto unimplemented;
i32x4_bitmask:
    goto unimplemented;
i32x4_extend_low_i16x8_s:
    goto unimplemented;
i32x4_extend_high_i16x8_s:
    goto unimplemented;
i32x4_extend_low_i16x8_u:
    goto unimplemented;
i32x4_extend_high_i16x8_u:
    goto unimplemented;
i32x4_shl:
    goto unimplemented;
i32x4_shr_s:
    goto unimplemented;
i32x4_shr_u:
    goto unimplemented;
i32x4_add:
    goto unimplemented;
i32x4_sub:
    goto unimplemented;
i32x4_mul:
    goto unimplemented;
i32x4_min_s:
    goto unimplemented;
i32x4_min_u:
    goto unimplemented;
i32x4_max_s:
    goto unimplemented;
i32x4_max_u:
    goto unimplemented;
i32x4_dot_i16x8_s:
    goto unimplemented;
i32x4_extmul_low_i16x8_s:
    goto unimplemented;
i32x4_extmul_high_i16x8_s:
    goto unimplemented;
i32x4_extmul_low_i16x8_u:
    goto unimplemented;
i32x4_extmul_high_i16x8_u:
    goto unimplemented;
i64x2_abs:
    goto unimplemented;
i64x2_neg:
    goto unimplemented;
i64x2_all_true:
    goto unimplemented;
i64x2_bitmask:
    goto unimplemented;
i64x2_extend_low_i32x4_s:
    goto unimplemented;
i64x2_extend_high_i32x4_s:
    goto unimplemented;
i64x2_extend_low_i32x4_u:
    goto unimplemented;
i64x2_extend_high_i32x4_u:
    goto unimplemented;
i64x2_shl:
    goto unimplemented;
i64x2_shr_s:
    goto unimplemented;
i64x2_shr_u:
    goto unimplemented;
i64x2_add:
    goto unimplemented;
i64x2_sub:
    goto unimplemented;
i64x2_mul:
    goto unimplemented;
i64x2_eq:
    goto unimplemented;
i64x2_ne:
    goto unimplemented;
i64x2_lt_s:
    goto unimplemented;
i64x2_gt_s:
    goto unimplemented;
i64x2_le_s:
    goto unimplemented;
i64x2_ge_s:
    goto unimplemented;
i64x2_extmul_low_i32x4_s:
    goto unimplemented;
i64x2_extmul_high_i32x4_s:
    goto unimplemented;
i64x2_extmul_low_i32x4_u:
    goto unimplemented;
i64x2_extmul_high_i32x4_u:
    goto unimplemented;
f32x4_abs:
    goto unimplemented;
f32x4_neg:
    goto unimplemented;
f32x4_sqrt:
    goto unimplemented;
f32x4_add:
    goto unimplemented;
f32x4_sub:
    goto unimplemented;
f32x4_mul:
    goto unimplemented;
f32x4_div:
    goto unimplemented;
f32x4_min:
    goto unimplemented;
f32x4_max:
    goto unimplemented;
f32x4_pmin:
    goto unimplemented;
f32x4_pmax:
    goto unimplemented;
f64x2_abs:
    goto unimplemented;
f64x2_neg:
    goto unimplemented;
f64x2_sqrt:
    goto unimplemented;
f64x2_add:
    goto unimplemented;
f64x2_sub:
    goto unimplemented;
f64x2_mul:
    goto unimplemented;
f64x2_div:
    goto unimplemented;
f64x2_min:
    goto unimplemented;
f64x2_max:
    goto unimplemented;
f64x2_pmin:
    goto unimplemented;
f64x2_pmax:
    goto unimplemented;
i32x4_trunc_sat_f32x4_s:
    goto unimplemented;
i32x4_trunc_sat_f32x4_u:
    goto unimplemented;
f32x4_convert_i32x4_s:
    goto unimplemented;
f32x4_convert_i32x4_u:
    goto unimplemented;
i32x4_trunc_sat_f64x2_s_zero:
    goto unimplemented;
i32x4_trunc_sat_f64x2_u_zero:
    goto unimplemented;
f64x2_convert_low_i32x4_s:
    goto unimplemented;
f64x2_convert_low_i32x4_u:
    goto unimplemented;
}

static void wasm_vm_disassemble(WASMVirtualMachine* vm, WASMInstruction instruction)
{
    WASMModule const* mod = vm->module;
    // FIXME: Check if disassembly is enabled.
    switch (instruction.opcode) {
    case WASMOpcode_unreachable:
    case WASMOpcode_nop:
    case WASMOpcode_return_:
    case WASMOpcode_i32_sub:
    case WASMOpcode_i32_add:
    case WASMOpcode_drop:
    case WASMOpcode_i32_and:
    case WASMOpcode_i32_eq:
    case WASMOpcode_i32_eqz:
        debugf("%s", wasm_opcode_string(instruction.opcode));
        break;
    case WASMOpcode_global_get:
    case WASMOpcode_global_set:
    case WASMOpcode_local_get:
    case WASMOpcode_local_set:
    case WASMOpcode_i32_const:
        debugf("%s %d(%u)", wasm_opcode_string(instruction.opcode), instruction.a.i32, instruction.a.u32);
        break;
    case WASMOpcode_i32_load:
    case WASMOpcode_i32_store:
    case WASMOpcode_block:
        debugf("%s %d(%u) %d(%u)", wasm_opcode_string(instruction.opcode), instruction.a.i32, instruction.a.u32, instruction.b.i32, instruction.b.u32);
        break;
    case WASMOpcode_i64_const:
        debugf("%s %ld(%lu)", wasm_opcode_string(instruction.opcode), instruction.a.i64, instruction.a.u64);
        break;
    case WASMOpcode_f32_const:
        debugf("%s %f", wasm_opcode_string(instruction.opcode), instruction.a.f32);
        break;
    case WASMOpcode_f64_const:
        debugf("%s %f", wasm_opcode_string(instruction.opcode), instruction.a.f64);
        break;
        debugf("%s %u", wasm_opcode_string(instruction.opcode), instruction.a.u32);
        break;
    case WASMOpcode_call: {
        if (instruction.a.u32 < mod->import_section.count) {
            WASMImport const* import = &mod->import_section.items[instruction.a.u32];
            debugf("%s '%.*s'.'%.*s'(%u)",
                wasm_opcode_string(instruction.opcode),
                import->mod.count,
                import->mod.items,
                import->name.count,
                import->name.items,
                instruction.a.u32
            );
        } else {
            debugf("%s %u", wasm_opcode_string(instruction.opcode), instruction.a.i32);
        }
        break;
    }
    default:
        errorf("unimplemented opcode: %s", wasm_opcode_string(instruction.opcode));
        UNIMPLEMENTED();
    }
}
