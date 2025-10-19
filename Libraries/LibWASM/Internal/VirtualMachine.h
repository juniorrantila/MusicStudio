#pragma once
#include "../VirtualMachine.h"
#include "./Opcode.h"

#include <Basic/Bits.h>

/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.4.2.8
/// 4.2.8 Memory Instances
/// The length of the vector always is a multiple of the WebAssembly page size, which is defined to be the constant
/// 65536 – abbreviated 64 Ki.
static const u64 wasm_page_size = 65536;

/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.2.2.5
/// 2.2.5 Names
/// Names are sequences of characters, which are scalar values as defined by Unicode (Section 2.4).
/// Due to the limitations of the binary format, the length of a name is bounded by the length of its UTF-8 encoding
typedef struct WASMName {
    char items[63];
    u8 count;
} WASMName;

/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.3.1
/// 5.3.1 Number Types
/// Number types are encoded by a single byte.
typedef enum WASMNumType : u8 {
    WASMNumType_i32 = 0x7F,
    WASMNumType_i64 = 0x7E,
    WASMNumType_f32 = 0x7D,
    WASMNumType_f64 = 0x7C,
} WASMNumType; /// numtype

/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.3.2
/// 5.3.2 Vector Types
/// Vector types are also encoded by a single byte.
typedef enum WASMVecType : u8 {
    WASMVecType_v128 = 0x7B,
} WASMVecType; /// vectype

/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.3.3
/// 5.3.3 Reference Types
/// Reference types are also encoded by a single byte.
typedef enum WASMRefType : u8 {
    WASMRefType_FuncRef   = 0x70, /// funcref
    WASMRefType_ExternRef = 0x6F, /// externref
} WASMRefType; /// reftype

/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.3.4
/// 5.3.4 Value Types
/// Value types are encoded with their respective encoding as a number type, vector type, or reference type.
///
/// NOTE: Value types can occur in contexts where type indices are also allowed, such as in the case of block types.
/// Thus, the binary format for types corresponds to the signed LEB128 encoding of small negative s𝑁 values, so
/// that they can coexist with (positive) type indices in the future
typedef enum WASMValType : u8 {
    WASMValType_i32 = WASMNumType_i32,
    WASMValType_i64 = WASMNumType_i64,
    WASMValType_f32 = WASMNumType_f32,
    WASMValType_f64 = WASMNumType_f64,

    WASMValType_v128 = WASMVecType_v128,

    WASMValType_FuncRef = WASMRefType_FuncRef,
    WASMValType_ExternRef = WASMRefType_ExternRef,
} WASMValType;
static_assert(sizeof(WASMValType) == 1);
C_API c_string wasm_valtype_string(WASMValType);

/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.3.5
/// 5.3.5 Result Types
/// Result types are encoded by the respective vectors of value types.
typedef struct WASMResultType {
    WASMValType items[15];
    u8 count;
} WASMResultType; /// resulttype
static_assert(sizeof(WASMResultType) == 16);

/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.3.6
/// 5.3.6 Function Types
/// Function types are encoded by the byte 0x60 followed by the respective vectors of parameter and result types.
typedef struct WASMFuncType {
    WASMResultType parameters;
    WASMResultType result_types;
} WASMFuncType; /// functype

/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.5.1
/// 5.5.1 Indices
/// All indices are encoded with their respective value.
typedef struct { u32 value; } WASMTypeIdx;
typedef struct { u32 value; } WASMFuncIdx;
typedef struct { u32 value; } WASMTableIdx;
typedef struct { u32 value; } WASMMemIdx;
typedef struct { u32 value; } WASMGlobalIdx;
typedef struct { u32 value; } WASMElemIdx;
typedef struct { u32 value; } WASMDataIdx;
typedef struct { u32 value; } WASMLocalIdx;
typedef struct { u32 value; } WASMLabelIdx;
typedef struct { u32 value; } WASMTagIdx;

typedef enum WASMExternTypeTag : u8 {
    WASMExternTypeTag_typeidx      = 0x00,
    WASMExternTypeTag_tableidx     = 0x01,
    WASMExternTypeTag_memidx       = 0x02,
    WASMExternTypeTag_globalidx    = 0x03,
    WASMExternTypeTag_tagtype      = 0x04,
} WASMExternTypeTag;
C_API c_string wasm_externtype_tag_string(WASMExternTypeTag);

/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.3.14
/// 5.3.14 External Types
/// External types are encoded by a distiguishing byte followed by an encoding of the respective form of type.
typedef struct WASMExternType {
    WASMExternTypeTag tag;
    union {
        WASMTypeIdx typeidx;
        WASMTableIdx tableidx;
        WASMMemIdx memidx;
        WASMGlobalIdx globalidx;
        WASMTagIdx tagidx;
        u32 raw_index; // NOTE: Not part of spec.
    };
} WASMExternType; // externtype

/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.3.6
/// 5.3.6 Function Types
/// Function types are encoded by the byte 0x60 followed by the respective vectors of parameter and result types.
typedef struct WASMImport {
    WASMName mod;
    WASMName name;
    WASMExternType externtype;
} WASMImport; /// import

/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.2.3.7
/// 2.3.7 Limits
/// Limits classify the size range of resizeable storage associated with memory types and table types.
/// If no maximum is given, the respective storage can grow to any size.
typedef struct WASMLimits {
    u32 min;
    u32 max;
} WASMLimits;

/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.2.3.8
/// 2.3.8 Memory Types
/// Memory types classify linear memories and their size range.
/// The limits constrain the minimum and optionally the maximum size of a memory. The limits are given in units of
/// page size.
typedef struct WASMMemType {
    WASMLimits limits;
} WASMMemType;

/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.2.3.10
/// 2.3.10 Global Types
/// Global types classify global variables, which hold a value and can either be mutable or immutable.
typedef struct WASMGlobalType {
    WASMValType valtype;
    bool is_mutable;
} WASMGlobalType;
static_assert(sizeof(WASMGlobalType) == 2);

// FIXME: Find spec for this.
typedef struct WASMConstVal {
    u64 initializer;
} WASMConstVal;

// FIXME: Find spec for this.
typedef struct WASMRefNull {} WASMRefNull;

// FIXME: Find spec for this.
typedef struct WASMRefFunc {
    WASMFuncIdx index;
} WASMRefFunc;

// FIXME: Find spec for this.
typedef struct WASMGlobalGet {
    WASMGlobalIdx index;
} WASMGlobalGet;

/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#section.2.4
/// Constant Expressions
/// * In a constant expression instr * end all instructions in instr * must be constant.
/// A constant instruction instr must be:
// NOLINTNEXTLINE(readability-enum-initial-value, cert-int09-c)
typedef enum WASMConstantInitializerExpressionKind : u8 {
    /// – either of the form 𝑡.const 𝑐,
    WASMConstantInitializerExpression_i32_const         = WASMOpcode_i32_const,
    WASMConstantInitializerExpression_i64_const         = WASMOpcode_i64_const,

    // WASMConstantInitializerExpressionKind_RefNull,      /// – or of the form ref.null,
    // WASMConstantInitializerExpressionKind_RefFunc,      /// – or of the form ref.func 𝑥,

    /// – or of the form global.get 𝑥, in which case 𝐶.globals[𝑥] must be a global type of the form const 𝑡.
    // WASMConstantInitializerExpressionKind_global_get_0_32 = WASMOpcode_global_get_0_32,
    // WASMConstantInitializerExpressionKind_global_get_32 = WASMOpcode_global_get_32,

    WASMConstantInitializerExpression_Invalid,
} WASMConstantInitializerExpressionKind;

/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#section.2.4
/// Constant Expressions
/// * In a constant expression instr * end all instructions in instr * must be constant.
/// A constant instruction instr must be:
typedef struct WASMConstantInitializerExpression {
    union {
        WASMConstVal constval;      /// – either of the form 𝑡.const 𝑐,
        // WASMRefNull refnull;        /// – or of the form ref.null,
        // WASMRefFunc reffunc;        /// – or of the form ref.func 𝑥,
        // WASMGlobalGet globalget;    /// – or of the form global.get 𝑥, in which case 𝐶.globals[𝑥] must be a global type of the form const 𝑡.t;
    };
    WASMConstantInitializerExpressionKind kind;
} WASMConstantInitializerExpression;

/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.2.3.10
/// 2.3.10 Global Types
/// Global types classify global variables, which hold a value and can either be mutable or immutable.
typedef struct WASMGlobal {
    WASMGlobalType type;
    WASMConstantInitializerExpression init;
} WASMGlobal;

typedef enum WASMExportDescKind : u8 {
    WASMExportDescKind_Func     = 0,
    WASMExportDescKind_Table    = 1,
    WASMExportDescKind_Memory   = 2,
    WASMExportDescKind_Global   = 3,
} WASMExportDescKind;
C_API c_string wasm_export_desc_kind_string(WASMExportDescKind);

/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.2.5.10
/// 2.5.10 Exports
typedef struct WASMExportDesc {
    union {
        WASMFuncIdx func;
        WASMTableIdx table;
        WASMMemIdx mem;
        WASMGlobalIdx global;
        u32 raw_index; // NOTE: Not in spec.
    };
    WASMExportDescKind kind;
} WASMExportDesc;

/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.2.5.10
/// 2.5.10 Exports
typedef struct WASMExport {
    WASMName name;
    WASMExportDesc desc;
} WASMExport;
static_assert(sizeof(WASMExport) == 72);

/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.5.2
/// 5.5.2 Sections
typedef enum WASMSectionID : u8 {
    WASMSectionID_Custom            = 0,
    WASMSectionID_TypeSection       = 1,
    WASMSectionID_ImportSection     = 2,
    WASMSectionID_FunctionSection   = 3,
    WASMSectionID_TableSection      = 4,
    WASMSectionID_MemorySection     = 5,
    WASMSectionID_GlobalSection     = 6,
    WASMSectionID_ExportSection     = 7,
    WASMSectionID_StartSection      = 8,
    WASMSectionID_ElementSection    = 9,
    WASMSectionID_CodeSection       = 10,
    WASMSectionID_DataSection       = 11,
    WASMSectionID_DataCountSection  = 12,
} WASMSectionID;

/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.5.4
/// 5.5.4 Type Section
/// The type section has the id 1. It decodes into a vector of function types that represent the types component of a module.
typedef struct WASMTypeSection {
    WASMFuncType items[255];
    u64 : 64;
    u64 : 64;
    u64 : 64;
    u64 : 56;
    u8 count;
} WASMTypeSection;
static_assert(sizeof(WASMTypeSection) == 8192);

/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.5.5
/// The import section has the id 2. It decodes into the list of imports of a module.
typedef struct WASMImportSection {
    WASMImport items[30];
    u64 : 64;
    u64 : 56;
    u8 count;
} WASMImportSection;
static_assert(sizeof(WASMImportSection) == 4096);

/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.5.6
/// 5.5.6 Function Section
/// The function section has the id 3. It decodes into a vector of type indices that represent the type fields of the
/// functions in the funcs component of a module. The locals and body fields of the respective functions are encoded
/// separately in the code section.
typedef struct WASMFunctionSection {
    WASMTypeIdx items[255];
    u8 count;
} WASMFunctionSection;
static_assert(sizeof(WASMFunctionSection) == 1024);

/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.5.8
/// 5.5.8 Memory Section
/// The memory section has the id 5. It decodes into a vector of memories that represent the mems component of a
/// module.
/// NOTE: In the current version of WebAssembly, at most one memory may be defined or imported in a single module,
/// and all constructs implicitly reference this memory 0. This restriction may be lifted in future versions.
typedef struct WASMMemorySection {
    WASMMemType items[1];
    u8 count;
} WASMMemorySection;
static_assert(sizeof(WASMMemorySection) == 12);

/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.5.9
/// 5.5.9 Global Section
/// The global section has the id 6. It decodes into a vector of globals that represent the globals component of a module.
typedef struct WASMGlobalSection {
    WASMGlobal items[255];
    u8 count;
} WASMGlobalSection;

/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.2.5.10
/// 2.5.10 Exports section
/// The exports component of a module defines a set of exports that become accessible to the host environment once
/// the module has been instantiated.
typedef struct WASMExportSection {
    WASMExport items[28];
    u64 : 64;
    u64 : 64;
    u64 : 64;
    u64 : 56;
    u8 count;
} WASMExportSection;
static_assert(sizeof(WASMExportSection) == 2048);

static constexpr u64 wasm_locals_per_type_max = 255;
typedef struct WASMLocals {
    u8 count;
    WASMValType type;
} WASMLocals;
static_assert(sizeof(WASMLocals) == 2);

typedef struct WASMLocalsArray {
    WASMLocals items[7];
    u16 count;
} WASMLocalsVector;
static_assert(sizeof(WASMLocalsVector) == 16);

typedef union WASMInstructionParameter {
    i32 i32;
    u32 u32;
    i64 i64;
    u64 u64;
    f32 f32;
    f64 f64;
} WASMInstructionParameter;

typedef struct WASMInstruction {
    WASMOpcode opcode;
    WASMInstructionParameter a;
    WASMInstructionParameter b;
} WASMInstruction;
static_assert(sizeof(WASMInstruction) == 24);

typedef struct WASMExpression {
    WASMInstruction items[8191];
    u64 : 48;
    u16 count;
} WASMExpression;
static_assert(sizeof(WASMExpression) == 196592);

typedef struct WASMCode {
    u64 size;
    WASMLocalsVector locals;
    WASMExpression expression;
} WASMCode;
static_assert(sizeof(WASMCode) == 196616);

/// https://webassembly.github.io/spec/core/_download/WebAssembly.pdf#subsection.5.5.13
/// 5.5.13 Code Section
typedef struct WASMCodeSection {
    WASMCode items[255];
    u64 : 64;
    u64 : 56;
    u8 count;
} WASMCodeSection;
static_assert(sizeof(WASMCodeSection) == 50137096);

typedef struct {
    c_string name;
    void* address;
} WASMNativeVariable;

typedef struct {
    WASMNativeFunctionCallback callback;
    c_string mod;
    c_string name;
    u32 inputs;
    u32 outputs;
} WASMNativeFunction;

typedef struct WASMGlobals {
    u32 items[1024];
} WASMGlobals;

typedef struct WASMMemory {
    u8 items[128 * KiB];
} WASMMemory;

typedef struct WASMModule {
    WASMTypeSection type_section;
    WASMImportSection import_section;
    WASMFunctionSection function_section;
    WASMMemorySection memory_section;
    WASMGlobalSection global_section;
    WASMExportSection export_section;
    WASMCodeSection code_section;

    struct {
        u32 count;
        WASMNativeVariable items[256];
    } native_variables;

    WASMNativeFunction native_functions[256];

    WASMGlobals globals;
    WASMMemory memory;
} WASMModule;
static_assert(sizeof(WASMModule) == 50306064);
static_assert(alignof(WASMModule) == 8);

struct WASMVirtualMachine {
    WASMModule const* module;

    struct {
        u32 count;
        u32 items[32 * KiB];
    } stack;

    struct {
        u32 count;
        u32 items[32 * KiB];
    } locals_stack;

    WASMGlobals globals;
    WASMMemory memory;
};
static_assert(sizeof(WASMVirtualMachine) == 397328);
static_assert(alignof(WASMVirtualMachine) == 8);
