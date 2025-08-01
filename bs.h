#if 0
set -e

BASE_PATH=`realpath $(dirname $0)`
TOOLCHAIN_DIR=$BASE_PATH/Toolchain
PROG_NAME=$(basename $0)

$TOOLCHAIN_DIR/bootstrap.sh > /dev/stderr
export PATH="$TOOLCHAIN_DIR/Tools/bin:$PATH"

if [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    echo "USAGE: $PROG_NAME [flags]"
    echo ""
    echo "FLAGS:"
    echo "        --no-echo    do not echo message about where files where generated"
    echo ""
    exit 0
fi

ccache clang -fcolor-diagnostics -std=c++17 -c -emit-llvm -xc++ $0 -o /tmp/bootstrap
lli /tmp/bootstrap
rm -f build/compile_commands.json

if [ "$1" != "--no-echo" ]; then
    echo '#' $PROG_NAME: created build/build.ninja
    echo '#' $PROG_NAME: to build the project, run ninja -C build
    echo '#' $PROG_NAME: if ninja is not in your PATH, run the command below:
    echo
    echo source $TOOLCHAIN_DIR/env
    echo
fi

exit 0
#endif
#ifndef BS_H
#define BS_H

#define USE_SANITIZERS
// #define USE_REALTIME_SANITIZER
// #define USE_THREAD_SANITIZER

#define OPTIMIZE_LEVEL "-O2"

#include <stdlib.h>
#include <libgen.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>

#define MAX_ENTRIES 256

typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef signed long i64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long u64;

_Static_assert(sizeof(i8) == 1, "");
_Static_assert(sizeof(i16) == 2, "");
_Static_assert(sizeof(i32) == 4, "");
_Static_assert(sizeof(i64) == 8, "");

_Static_assert(sizeof(u8) == 1, "");
_Static_assert(sizeof(u16) == 2, "");
_Static_assert(sizeof(u32) == 4, "");
_Static_assert(sizeof(u64) == 8, "");

typedef __SIZE_TYPE__ usize;
#define unsigned signed
typedef __SIZE_TYPE__ isize;
#undef unsigned

typedef usize uptr;
typedef isize iptr;

typedef float f32;
typedef double f64;

typedef char const* c_string;

typedef struct StringView {
    char const* characters;
    usize size;
} StringView;

typedef struct {
    usize size;
    StringView items[MAX_ENTRIES];
} StringSet;

static inline StringView string_view(c_string string);
static inline bool string_view_equal(StringView a, StringView b);
static inline bool string_set_has(StringSet const* set, StringView item);
static inline void string_set_add(StringSet* set, StringView item);

typedef enum {
    TargetKind_Binary,
    TargetKind_SharedLibrary,
    TargetKind_StaticLibrary,
    TargetKind_Targets,
    TargetKind_UniversalBinary,
    TargetKind_UniversalLibrary,
} TargetKind;

struct BinaryArgs;
struct LibraryArgs;
struct UniversalBinaryArgs;
struct Targets;
typedef struct {
    c_string name;
    c_string file;
    c_string base_dir;
    union {
        struct BinaryArgs* binary;
        struct LibraryArgs* library;
        struct Targets* targets;
        struct UniversalBinaryArgs* universal_binary;
        struct UniversalLibraryArgs* universal_library;
    };
    TargetKind kind;
} Target;

typedef struct {
    c_string entries[MAX_ENTRIES];
} Strings;

typedef struct Targets {
    Target entries[MAX_ENTRIES];
} Targets;

typedef struct DynTargets {
    usize size;
    Targets targets;
} DynTargets;

typedef struct TargetTripple {
    c_string arch;
    c_string abi;
    c_string os;
} TargetTriple;

typedef struct BinaryArgs {
    Strings srcs;
    Strings compile_flags;
    Strings linker_flags;
    TargetTriple target_triple;
    Targets deps;
} BinaryArgs;

typedef struct LibraryArgs {
    Strings srcs;
    Strings exported_headers;
    c_string header_namespace;
    Strings compile_flags;
    Strings linker_flags;
    TargetTriple target_triple;
    c_string link_style;
    Targets deps;
} LibraryArgs;

typedef struct UniversalBinaryArgs {
    Targets bins;
} UniversalBinaryArgs;

typedef struct UniversalLibraryArgs {
    Targets libs;
} UniversalLibraryArgs;

static inline Strings default_cxx_args(void);

template <typename T, usize Count>
static inline usize len(T const (& items)[Count]);

template <typename T, usize Count>
static inline usize capacity(T const (& items)[Count]);

template <typename T, usize Count, usize Count2>
static inline void cat(T (&items)[Count], T const (& other)[Count2]);

template <typename F>
static inline auto target(F callback, c_string file = __builtin_FILE());

static inline Target cc_binary(c_string name, BinaryArgs args, c_string file = __builtin_FILE());
static inline Target cc_library(c_string name, LibraryArgs args, c_string file = __builtin_FILE());

static inline Target universal_binary(c_string name, UniversalBinaryArgs args, c_string file = __builtin_FILE());
static inline Target universal_library(c_string name, UniversalLibraryArgs args, c_string file = __builtin_FILE());

static inline Target cpp_bundle_library(c_string name, LibraryArgs args, c_string file = __builtin_FILE());

static inline void emit_ninja(FILE* output, Target target);

static inline void recurse_targets(Target target, void* user, void(*callback)(void* user, Target target));

static inline Targets flatten_targets(Target target);

static inline c_string system_os(void);
static inline c_string system_arch(void);
static inline c_string system_abi(void);

static inline TargetTriple system_target_triple(void);
static inline TargetTriple wasm_target_triple(void);
static inline c_string target_triple_string(TargetTriple triple);
static inline TargetTriple target_triple(Target target);
static inline TargetTriple apple_arm_target_triple(void);
static inline TargetTriple apple_x86_target_triple(void);

static inline void setup(c_string, c_string file = __builtin_FILE());

static inline void dyn_targets_add(DynTargets* targets, Target target);
static inline void dyn_targets_add_g(void* targets, Target target);

static inline void mkdir_p(char* path, mode_t mode);

template <typename T, usize Count>
static inline usize len(T const (& items)[Count])
{
    usize i = 0;
    for (; i < Count; i++) {
        char zeroes[sizeof(T)];
        memset(zeroes, 0, sizeof(zeroes));
        T item {};
        if (memcmp(&items[i], &item, sizeof(T)) == 0)
            break;
    }
    return i;
}

template <typename T, usize Count>
static inline usize capacity(T const (& items)[Count])
{
    (void)items;
    return Count;
}

template <typename T, usize Count, usize Count2>
static inline void cat(T (&items)[Count], T const (& other)[Count2])
{
    static_assert(Count >= Count2);
    usize c = len(items);
    usize c2 = len(other);
    assert(Count - c > c2);
    memcpy(items + c, &other, sizeof(T) * c2);
}

template <typename T, usize Count>
static inline void append(T (&items)[Count], T value)
{
    usize l = len(items);
    assert(l + 1 < Count);
    items[l] = value;
}

static inline void add_sanitizers(Strings* flags, TargetTriple triple)
{
    if (!triple.arch) triple.arch = system_arch();
    if (!triple.os) triple.os = system_os();
    if (!triple.abi) triple.abi = system_abi();
    if (strcmp(triple.arch, "wasm32") == 0)
        return;
#ifndef USE_SANITIZERS
    return;
#endif

#ifdef USE_REALTIME_SANITIZER
    append(flags->entries, "-fsanitize=realtime");
    return; // Can't use realtime sanitizer with anything else.
#endif

#ifdef USE_THREAD_SANITIZER
    append(flags->entries, "-fsanitize=thread");
    return; // Can't use thread sanitizer with anything else.
#endif

    append(flags->entries, "-fsanitize=address");
    if (strcmp(triple.os, "darwin") != 0) {
        append(flags->entries, "-fsanitize=memory");
    }
    // append(flags->entries, "-fsanitize=type");
    // append(flags->entries, "-fsanitize=undefined");
    append(flags->entries, "-fsanitize=null");
    append(flags->entries, "-fsanitize=bool");
    append(flags->entries, "-fsanitize=builtin");
    append(flags->entries, "-fsanitize=bounds");
    append(flags->entries, "-fsanitize=enum");
    append(flags->entries, "-fsanitize=float-cast-overflow");
    append(flags->entries, "-fsanitize=integer");
    append(flags->entries, "-fsanitize-address-use-after-scope");
    // append(flags->entries, "-fsanitize-stats");
}

static inline Targets all_targets_deps;
static inline usize all_targets_count = 0;
static inline Target all_targets = {
    .name = "all",
    .file = "",
    .base_dir = "",
    .targets = &all_targets_deps,
    .kind = TargetKind_Targets,
};
static inline Target cc_binary(c_string name, BinaryArgs args, c_string file)
{
    c_string base_dir = strdup(dirname(strdup(file)));
    BinaryArgs* res = (BinaryArgs*)malloc(sizeof(args));
    *res = args;

    add_sanitizers(&res->compile_flags, res->target_triple);
    add_sanitizers(&res->linker_flags, res->target_triple);

    if (res->target_triple.abi == 0) {
        res->target_triple.abi = system_abi();
    }
    if (res->target_triple.os == 0) {
        res->target_triple.os = system_os();
    }
    if (res->target_triple.arch == 0) {
#ifndef __APPLE__
        res->target_triple.arch = system_arch();
#else
        res->target_triple.arch = "arm64";
        BinaryArgs* x86 = (BinaryArgs*)malloc(sizeof(args));
        *x86 = *res;
        x86->target_triple.arch = "x86_64";
        Target arm_target = {
            .name = name,
            .file = file,
            .base_dir = base_dir,
            .binary = res,
            .kind = TargetKind_Binary,
        };
        Target x86_target = {
            .name = name,
            .file = file,
            .base_dir = base_dir,
            .binary = x86,
            .kind = TargetKind_Binary,
        };
        assert(all_targets_count < MAX_ENTRIES - 1);
        all_targets_deps.entries[all_targets_count++] = arm_target;
        assert(all_targets_count < MAX_ENTRIES - 1);
        all_targets_deps.entries[all_targets_count++] = x86_target;
        return universal_binary(name, {
            .bins = {
                arm_target,
                x86_target,
            },
        }, file);
#endif
    }

    Target target = {
        .name = name,
        .file = file,
        .base_dir = base_dir,
        .binary = res,
        .kind = TargetKind_Binary,
    };
    assert(all_targets_count < MAX_ENTRIES - 1);
    all_targets_deps.entries[all_targets_count++] = target;
    return target;
}

static inline TargetKind parse_link_style(c_string link_style)
{
    if (strcmp(link_style, "both") == 0)
        return TargetKind_SharedLibrary;
    return TargetKind_StaticLibrary;
}

static inline Target cc_library(c_string name, LibraryArgs args, c_string file)
{
    TargetKind link_style = parse_link_style(args.link_style);
    c_string base_dir = strdup(dirname(strdup(file)));
    LibraryArgs* res = (LibraryArgs*)malloc(sizeof(args));
    *res = args;

    add_sanitizers(&res->compile_flags, res->target_triple);
    // add_sanitizers(&res->linker_flags, res->target_triple);

    if (res->target_triple.abi == 0) {
        res->target_triple.abi = system_abi();
    }
    if (res->target_triple.os == 0) {
        res->target_triple.os = system_os();
    }
    if (res->target_triple.arch == 0) {
#ifndef __APPLE__
        res->target_triple.arch = system_arch();
#else
        res->target_triple.arch = "arm64";
        LibraryArgs* x86 = (LibraryArgs*)malloc(sizeof(args));
        *x86 = *res;
        x86->target_triple.arch = "x86_64";
        Target arm_target = {
            .name = name,
            .file = file,
            .base_dir = base_dir,
            .library = res,
            .kind = link_style,
        };
        Target x86_target = {
            .name = name,
            .file = file,
            .base_dir = base_dir,
            .library = x86,
            .kind = link_style,
        };
        assert(all_targets_count < MAX_ENTRIES - 1);
        all_targets_deps.entries[all_targets_count++] = arm_target;
        assert(all_targets_count < MAX_ENTRIES - 1);
        all_targets_deps.entries[all_targets_count++] = x86_target;
        return universal_library(name, {
            .libs = {
                arm_target,
                x86_target,
            },
        }, file);
#endif
    }

    Target target = {
        .name = name,
        .file = file,
        .base_dir = base_dir,
        .library = res,
        .kind = link_style,
    };
    assert(all_targets_count < MAX_ENTRIES - 1);
    all_targets_deps.entries[all_targets_count++] = target;
    return target;
}

static inline Target universal_binary(c_string name, UniversalBinaryArgs args, c_string file)
{
    c_string base_dir = strdup(dirname(strdup(file)));
    UniversalBinaryArgs* res = (UniversalBinaryArgs*)malloc(sizeof(args));
    *res = args;
    Target target = {
        .name = name,
        .file = file,
        .base_dir = base_dir,
        .universal_binary = res,
        .kind = TargetKind_UniversalBinary,
    };
    assert(all_targets_count < MAX_ENTRIES - 1);
    all_targets_deps.entries[all_targets_count++] = target;
    return target;
}

static inline Target universal_library(c_string name, UniversalLibraryArgs args, c_string file)
{
    c_string base_dir = strdup(dirname(strdup(file)));
    UniversalLibraryArgs* res = (UniversalLibraryArgs*)malloc(sizeof(args));
    *res = args;
    Target target = {
        .name = name,
        .file = file,
        .base_dir = base_dir,
        .universal_library = res,
        .kind = TargetKind_UniversalLibrary,
    };
    assert(all_targets_count < MAX_ENTRIES - 1);
    all_targets_deps.entries[all_targets_count++] = target;
    return target;
}

typedef struct Variable {
    c_string name;
    c_string default_value;
} Variable;

typedef struct TargetRule {
    c_string name;
    c_string command;
    c_string description;

    Variable variables[8];
} TargetRule;

static inline usize all_rules_count = 0;
static inline TargetRule all_rules[64];
static inline TargetRule ninja_rule(TargetRule rule)
{
    assert(all_rules_count < 64);
    all_rules[all_rules_count++] = rule;
    return rule;
}

static inline TargetRule cxx_rule = ninja_rule({
    .name = "cxx",
    .command = "$bin/ccache $bin/clang++ -target $target $args -MD -MQ $out -MF $out.d -o $out -c $in",
    .description = "Compiling $language object $out",
    .variables = {
        (Variable){
            .name = "out",
            .default_value = nullptr,
        },
        (Variable){
            .name = "in",
            .default_value = nullptr,
        },
        (Variable){
            .name = "target",
            .default_value = nullptr,
        },
        (Variable){
            .name = "deps",
            .default_value = "gcc",
        },
        (Variable){
            .name = "args",
            .default_value = nullptr,
        },
        (Variable){
            .name = "depfile",
            .default_value = "$out.d",
        },
        (Variable){
            .name = "language",
            .default_value = nullptr,
        },
    },
});

static inline auto send_file_rule = ninja_rule({
    .name = "send-file",
    .command = "$uni_bin/send-file $in $out",
    .description = "Moving $in => $out",
    .variables = {
        (Variable){
            .name = "out",
            .default_value = nullptr,
        },
        (Variable){
            .name = "in",
            .default_value = nullptr,
        },
    },
});

typedef struct Language {
    c_string name;
    c_string pretty_name;
    c_string extension;
} Language;

static inline Language* language_from_filename(c_string name)
{
    static Language languages[] = {
        {
            .name = "cpp",
            .pretty_name = "C++",
            .extension = ".cpp",
        },
        {
            .name = "c",
            .pretty_name = "C",
            .extension = ".c",
        },
        {
            .name = "objcpp",
            .pretty_name = "Objective-C++",
            .extension = ".mm",
        },
        {
            .name = "objc",
            .pretty_name = "Objective-C",
            .extension = ".m",
        },
        {
            .name = "asm",
            .pretty_name = "assembly",
            .extension = ".asm",
        },
        {
            .name = "asm",
            .pretty_name = "arm64 assembly",
            .extension = ".arm64",
        },
        {
            .name = "asm",
            .pretty_name = "arm assembly",
            .extension = ".arm",
        },
        {
            .name = "asm",
            .pretty_name = "x86 assembly",
            .extension = ".x86",
        },
        {
            .name = "asm",
            .pretty_name = "i386 assembly",
            .extension = ".i386",
        },
    };
    usize name_len = strlen(name);
    for (usize i = 0; i < capacity(languages); i++) {
        Language* language = &languages[i];
        usize extension_len = strlen(language->extension);
        if (name_len < extension_len) {
            continue;
        }
        c_string ext = name + name_len - extension_len;
        if (strcmp(ext, language->extension) == 0) {
            return language;
        }
    }
    return nullptr;
}

static inline TargetRule merge_object_rule = ninja_rule({
    .name = "merge-object",
    .command = "$ld $extra_flags $link_args -o $out $in",
    .description = "Linking static target $out",
    .variables = {
        (Variable){
            .name = "out",
            .default_value = nullptr,
        },
        (Variable){
            .name = "in",
            .default_value = nullptr,
        },
        (Variable){
            .name = "link_args",
            .default_value = nullptr,
        },
        (Variable){
            .name = "extra_flags",
            .default_value = nullptr,
        },
    },
});

static inline TargetRule link_shared_rule = ninja_rule({
    .name = "link-shared",
    .command = "$ld $extra_flags $link_args -o $out $in",
    .description = "Linking shared target $out",
    .variables = {
        (Variable){
            .name = "out",
            .default_value = nullptr,
        },
        (Variable){
            .name = "in",
            .default_value = nullptr,
        },
        (Variable){
            .name = "link_args",
            .default_value = nullptr,
        },
        (Variable){
            .name = "extra_flags",
            .default_value = nullptr,
        },
    },
});

static inline TargetRule binary_link_rule = ninja_rule({
    .name = "link-binary",
    .command = "$bin/clang++ -target $target -o $out $in $link_args",
    .description = "Linking binary target $out",
    .variables = {
        (Variable){
            .name = "out",
            .default_value = nullptr,
        },
        (Variable){
            .name = "in",
            .default_value = nullptr,
        },
        (Variable){
            .name = "target",
            .default_value = nullptr,
        },
        (Variable){
            .name = "link_args",
            .default_value = nullptr,
        },
    },
});

static inline TargetRule universal_binary_rule = ninja_rule({
    .name = "universal",
    .command = "$bin/llvm-lipo -create -output $out $in",
    .description = "Creating universal $out",
    .variables = {
        (Variable){
            .name = "out",
            .default_value = nullptr,
        },
        (Variable){
            .name = "in",
            .default_value = nullptr,
        },
    },
});

static inline TargetRule compdb_rule = ninja_rule({
    .name = "compdb",
    .command = "$bin/ninja -t compdb > compile_commands.json",
    .description = "Emitting compdb",
    .variables = {},
});

static inline TargetRule reconfigure_rule = ninja_rule({
    .name = "reconfigure",
    .command = "cd .. && $configure --no-echo",
    .description = "Reconfiguring",
    .variables = {
        (Variable) {
            .name = "configure",
            .default_value = 0,
        },
        (Variable) {
            .name = "generator",
            .default_value = "generator",
        },
    }
});

static inline bool is_library(TargetKind kind)
{
    switch (kind) {
    case TargetKind_SharedLibrary:
    case TargetKind_StaticLibrary:
        return true;
    case TargetKind_Binary:
    case TargetKind_Targets:
    case TargetKind_UniversalBinary:
    case TargetKind_UniversalLibrary:
        return false;
    }
}

static inline c_string g_root_file;
static inline c_string g_build_dir;
static inline void setup(c_string build_dir, c_string file)
{
    g_root_file = file;
    g_build_dir = build_dir;
    mkdir(build_dir, 0777);

    c_string triple = target_triple_string(system_target_triple());

    char* lib_dir = 0;
    assert(asprintf(&lib_dir, "%s/lib", build_dir) > 0);
    char* system_lib = 0;
    assert(asprintf(&system_lib, "%s/lib", triple) > 0);

    char* bin_dir = 0;
    assert(asprintf(&bin_dir, "%s/bin", build_dir) > 0);
    char* system_bin = 0;
    assert(asprintf(&system_bin, "%s/bin", triple) > 0);

    (void)remove(bin_dir);
    symlink(system_bin, bin_dir);

    (void)remove(lib_dir);
    symlink(system_lib, lib_dir);
}

static inline void emit_ninja_rule(FILE* output, TargetRule const* rule)
{
    fprintf(output, "rule %s\n", rule->name);
    fprintf(output, "    command = %s\n", rule->command);
    fprintf(output, "    description = %s\n", rule->description);
    usize variable_count = len(rule->variables);
    for (usize i = 0; i < variable_count; i++) {
        Variable variable = rule->variables[i];
        if (variable.default_value) {
            fprintf(output, "    %s = %s\n", variable.name, variable.default_value);
        }
    }
    fprintf(output, "\n");
}

static inline void emit_ninja_build_binary(FILE* output, Target const* target)
{
    BinaryArgs* binary = target->binary;
    c_string triple = target_triple_string(binary->target_triple);
    c_string base_dir = target->base_dir;
    usize srcs_len = len(binary->srcs.entries);
    c_string name = target->name;

    Targets deps = flatten_targets({
        .name = "__deps__",
        .file = "",
        .base_dir = "",
        .targets = &binary->deps,
        .kind = TargetKind_Targets,
    });
    usize deps_len = len(deps.entries);

    fprintf(output, "build %s/bin/%s: link-binary", triple, name);
    for (usize i = 0; i < srcs_len; i++) {
        c_string src = binary->srcs.entries[i];
        fprintf(output, " %s/%s/%s.o", triple, base_dir, src);
    }
    StringSet seen_deps = (StringSet){};
    for (usize i = 1; i < deps_len; i++) {
        Target const* dep = &deps.entries[i];
        auto n = string_view(dep->name);
        if (string_set_has(&seen_deps, n)) {
            continue;
        }
        string_set_add(&seen_deps, n);
        fprintf(output, " %s/lib/%s.o", triple, dep->name);
    }
    fprintf(output, "\n");
    fprintf(output, "    target = %s\n", triple);
    usize link_args_len = len(binary->linker_flags.entries);
    if (link_args_len > 0) {
        fprintf(output, "    link_args =");
        for (usize i = 0; i < link_args_len; i++) {
            fprintf(output, " %s", binary->linker_flags.entries[i]);
        }
        fprintf(output, "\n");
    }
    fprintf(output, "\n");

    Strings const* args = &binary->compile_flags;
    usize args_len = len(args->entries);
    for (usize i = 0; i < srcs_len; i++) {
        seen_deps.size = 0;
        c_string src = binary->srcs.entries[i];
        Language* language = language_from_filename(src);
        if (!language) {
            fprintf(stderr, "Error: unknown language for %s\n", src);
            exit(1);
        }
        fprintf(output, "build %s/%s/%s.o: cxx ../%s/%s\n", triple, base_dir, src, base_dir, src);
        fprintf(output, "    language = %s\n", language->pretty_name);
        fprintf(output, "    target = %s\n", triple);
        fprintf(output, "    args = $default_%s_args", language->name);
        for (usize arg = 0; arg < args_len; arg++) {
            fprintf(output, " %s", args->entries[arg]);
        }
        for (usize dep_index = 1; dep_index < deps_len; dep_index++) {
            Target const* dep = &deps.entries[dep_index];
            if (is_library(dep->kind)) {
                StringView n = string_view(dep->name);
                if (string_set_has(&seen_deps, n)) {
                    continue;
                }
                string_set_add(&seen_deps, n);
                fprintf(output, " -Ins/%s -Ins/%s/%s", dep->name, triple, dep->name);
            }
        }
        fprintf(output, "\n");
        fprintf(output, "\n");
    }
}

static inline void emit_ninja_build_universal_binary(FILE* output, Target const* target)
{
    UniversalBinaryArgs* universal = target->universal_binary;

    c_string triple = target_triple_string(target_triple(*target));
    fprintf(output, "build %s/bin/%s: universal", triple, target->name);

    auto bins_len = len(universal->bins.entries);
    for (usize i = 0; i < bins_len; i++) {
        Target bin = universal->bins.entries[i];
        assert(bin.kind == TargetKind_Binary);
        c_string triple = target_triple_string(target_triple(bin));
        fprintf(output, " %s/bin/%s", triple, bin.name);
    }
    fprintf(output, "\n\n");
}

static inline void emit_ninja_build_universal_library(FILE* output, Target const* target)
{
    UniversalLibraryArgs* universal = target->universal_library;

    c_string triple = target_triple_string(target_triple(*target));
    fprintf(output, "build %s/lib/%s.o: universal", triple, target->name);

    auto bins_len = len(universal->libs.entries);
    for (usize i = 0; i < bins_len; i++) {
        Target lib = universal->libs.entries[i];
        assert(is_library(lib.kind));
        c_string triple = target_triple_string(target_triple(lib));
        fprintf(output, " %s/lib/%s.o", triple, lib.name);
    }
    fprintf(output, "\n\n");
}

static inline void mkdir_p(char* path, mode_t mode)
{
    usize len = strlen(path);
    for (usize i = 0; i < len; i++) {
        if (path[i] == '/') {
            path[i] = '\0';
            mkdir(path, mode);
            path[i] = '/';
        }
    }
    mkdir(path, mode);
}

static c_string shared_library_extension(LibraryArgs const* lib)
{
    c_string os = lib->target_triple.os;
    if (!os) os = system_os();
    if (strcmp(os, "linux") == 0) {
        return "so";
    }
    if (strcmp(os, "darwin") == 0) {
        return "dylib";
    }
    if (strcmp(os, "windows") == 0) {
        return "dll";
    }
    fprintf(stderr, "unknown os: %s\n", os);
    assert(false && "unknown os");
}

static c_string shared_library_prefix(LibraryArgs const* lib)
{
    c_string os = lib->target_triple.os;
    if (!os) os = system_os();
    if (strcmp(os, "linux") == 0) {
        return "lib";
    }
    if (strcmp(os, "darwin") == 0) {
        return "lib";
    }
    if (strcmp(os, "windows") == 0) {
        return "";
    }
    fprintf(stderr, "unknown os: %s\n", os);
    assert(false && "unknown os");
}

static inline void emit_ninja_build_static_library(FILE* output, Target const* target);
static inline void emit_ninja_build_shared_library(FILE* output, Target const* target)
{
    emit_ninja_build_static_library(output, target);

    LibraryArgs* library = target->library;
    c_string triple = target_triple_string(library->target_triple);
    c_string base_dir = target->base_dir;
    usize headers_len = len(library->exported_headers.entries);
    c_string name = target->name;

    char* dir = 0;
    asprintf(&dir, "%s/ns/%s/%s", g_build_dir, name, library->header_namespace);
    mkdir_p(dir, 0777);
    for (usize i = 0; i < headers_len; i++) {
        c_string header = library->exported_headers.entries[i];
        char* out = 0;
        asprintf(&out, "%s/%s", base_dir, header);
        c_string header_path = realpath(out, 0);
        if (!header_path) {
            (void)fprintf(stderr, "Error: Could not find '%s'\n", out);
            exit(1);
        }
        char* output_path = 0;
        asprintf(&output_path, "%s/%s", dir, header);
        mkdir_p(dirname(strdup(output_path)), 0777);
        symlink(header_path, output_path);
    }

    c_string library_prefix = shared_library_prefix(library);
    c_string library_extension = shared_library_extension(library);

    (void)fprintf(output, "build %s/lib/%s%s.%s: send-file  %s/lib/%s%s.%s.shadow\n\n", triple, library_prefix, name, library_extension, triple, library_prefix, name, library_extension);
    (void)fprintf(output, "build %s/lib/%s%s.%s.shadow: link-shared %s/lib/%s.o\n", triple, library_prefix, name, library_extension, triple, name);
    usize link_args_len = len(library->linker_flags.entries);
    if (link_args_len > 0) {
        (void)fprintf(output, "    link_args =");
        for (usize i = 0; i < link_args_len; i++) {
            (void)fprintf(output, " %s", library->linker_flags.entries[i]);
        }
    }
    if (strcmp(target_triple_string(library->target_triple), target_triple_string(wasm_target_triple())) == 0) {
        (void)fprintf(output, "    ld = $bin/wasm-ld\n");
        (void)fprintf(output, "    extra_flags = --strip-all\n");
    } else {
        // FIXME: Get linker from target triple.
#ifdef __APPLE__
        (void)fprintf(output, "    ld = ld\n"); // FIXME: This is system ld
#elif _WIN32
        (void)fprintf(output, "    ld = $bin/lld-link\n");
#else
        (void)fprintf(output, "    ld = $bin/ld.lld\n");
#endif
        (void)fprintf(output, "    extra_flags = -dylib -undefined dynamic_lookup\n");
    }
    (void)fprintf(output, "\n");
}

static inline void emit_ninja_build_static_library(FILE* output, Target const* target)
{
    LibraryArgs* library = target->library;
    c_string triple = target_triple_string(library->target_triple);
    c_string base_dir = target->base_dir;
    usize srcs_len = len(library->srcs.entries);
    usize headers_len = len(library->exported_headers.entries);
    c_string name = target->name;

    char* dir = 0;
    asprintf(&dir, "%s/ns/%s/%s", g_build_dir, name, library->header_namespace);
    mkdir_p(dir, 0777);
    for (usize i = 0; i < headers_len; i++) {
        c_string header = library->exported_headers.entries[i];
        char* out = 0;
        asprintf(&out, "%s/%s", base_dir, header);
        c_string header_path = realpath(out, 0);
        if (!header_path) {
            fprintf(stderr, "Error: Could not find '%s'\n", out);
            exit(1);
        }
        char* output_path = 0;
        asprintf(&output_path, "%s/%s", dir, header);
        mkdir_p(dirname(strdup(output_path)), 0777);
        symlink(header_path, output_path);
    }

    fprintf(output, "build %s/lib/%s.o: merge-object ", triple, name);
    for (usize i = 0; i < srcs_len; i++) {
        c_string src = library->srcs.entries[i];
        fprintf(output, "%s/%s/%s.o", triple, base_dir, src);
        if (i != srcs_len - 1) {
            fprintf(output, " ");
        }
    }
    fprintf(output, "\n");
    usize link_args_len = len(library->linker_flags.entries);
    if (link_args_len > 0) {
        fprintf(output, "    link_args =");
        for (usize i = 0; i < link_args_len; i++) {
            fprintf(output, " %s", library->linker_flags.entries[i]);
        }
        fprintf(output, "\n");
    }
    if (strcmp(target_triple_string(library->target_triple), target_triple_string(wasm_target_triple())) == 0) {
        fprintf(output, "    ld = $bin/wasm-ld\n");
        fprintf(output, "    extra_flags = --strip-all\n");
    } else {
        // FIXME: Get linker from target triple.
#ifdef __APPLE__
        fprintf(output, "    ld = ld\n"); // FIXME: This is system ld
#elif _WIN32
        fprintf(output, "    ld = $bin/lld-link\n");
#else
        fprintf(output, "    ld = $bin/ld.lld\n");
#endif
        fprintf(output, "    extra_flags = -r\n");
    }
    fprintf(output, "\n");

    Targets deps = flatten_targets({
        .name = "__deps__",
        .file = "",
        .base_dir = "",
        .targets = &library->deps,
        .kind = TargetKind_Targets,
    });
    usize deps_len = len(deps.entries);

    Strings const* args = &library->compile_flags;
    usize args_len = len(args->entries);
    for (usize i = 0; i < srcs_len; i++) {
        c_string src = library->srcs.entries[i];
        Language* language = language_from_filename(src);
        if (!language) {
            fprintf(stderr, "Error: unknown language for %s\n", src);
            exit(1);
        }
        fprintf(output, "build %s/%s/%s.o: cxx ../%s/%s\n", triple, base_dir, src, base_dir, src);
        fprintf(output, "    language = %s\n", language->pretty_name);
        fprintf(output, "    target = %s\n", triple);
        fprintf(output, "    args = $default_%s_args", language->name);
        for (usize arg = 0; arg < args_len; arg++) {
            fprintf(output, " %s", args->entries[arg]);
        }
        for (usize dep_index = 1; dep_index < deps_len; dep_index++) {
            Target const* dep = &deps.entries[dep_index];
            if (is_library(dep->kind)) {
                fprintf(output, " -Ins/%s -Ins/%s/%s", dep->name, triple, dep->name);
            }
        }
        fprintf(output, "\n");
        fprintf(output, "\n");
    }
}

static inline c_string extra_target_files[MAX_ENTRIES];

static inline void emit_ninja(FILE* output, Target target)
{
    (void)fprintf(output, "ninja_required_version = 1.8.2\n\n");

    (void)fprintf(output, "bin = ../Toolchain/Tools/bin\n");
    (void)fprintf(output, "uni_bin = universal-apple-darwin/bin\n");
    (void)fprintf(output, "\n");

    (void)fprintf(output, "default_cxx_args =");
    {
        Strings args = default_cxx_args();
        auto args_len = len(args.entries);
        for (usize i = 0; i < args_len; i++) {
            (void)fprintf(output, " %s", args.entries[i]);
        }
    }
    (void)fprintf(output, "\n\n");

    (void)fprintf(output, "default_cpp_args    = $default_cxx_args -xc++ -std=c++23\n");
    (void)fprintf(output, "default_c_args      = $default_cxx_args -xc -std=c23\n");
    (void)fprintf(output, "default_objc_args   = $default_cxx_args -xobjective-c -fobjc-arc -std=c23\n");
    (void)fprintf(output, "default_objcpp_args = $default_cxx_args -xobjective-c++ -fobjc-arc -std=c++23\n");
    (void)fprintf(output, "default_asm_args    = -xassembler-with-cpp\n");
    (void)fprintf(output, "\n");

    for (usize i = 0; i < all_rules_count; i++) {
        emit_ninja_rule(output, &all_rules[i]);
    }

    (void)fprintf(output, "build compile_commands.json: compdb\n\n");

    Targets targets = flatten_targets(target);
    usize targets_len = len(targets.entries);
    (void)fprintf(output, "build build.ninja: reconfigure ../%s", g_root_file);
    (void)fprintf(output, " ../%s", __FILE__);
    usize extra_target_files_len = len(extra_target_files);
    for (usize i = 0; i < extra_target_files_len; i++) {
        (void)fprintf(output, " ../%s", extra_target_files[i]);
    }
    for (usize i = 0; i < targets_len; i++) {
        Target const* target = &targets.entries[i];
        if (target->file[0] == '\0')
            continue;
        (void)fprintf(output, " ../%s", target->file);
    }
    (void)fprintf(output, "\n    configure = %s\n\n", g_root_file);

    for (usize i = 0; i < targets_len; i++) {
        Target const* target = &targets.entries[i];
        switch (target->kind) {
        case TargetKind_Binary:
            emit_ninja_build_binary(output, target);
            break;
        case TargetKind_SharedLibrary:
            emit_ninja_build_shared_library(output, target);
            break;
        case TargetKind_StaticLibrary:
            emit_ninja_build_static_library(output, target);
            break;
        case TargetKind_UniversalBinary:
            emit_ninja_build_universal_binary(output, target);
            break;
        case TargetKind_UniversalLibrary:
            emit_ninja_build_universal_library(output, target);
            break;
        case TargetKind_Targets:
            break;
        }
    }
}

template <typename F>
static inline auto target(F callback, c_string file)
{
    extra_target_files[len(extra_target_files)] = file;
    return callback();
}

static inline Targets const* target_deps(Target const* target)
{
    switch (target->kind) {
    case TargetKind_Binary: return &target->binary->deps;
    case TargetKind_SharedLibrary:
    case TargetKind_StaticLibrary: return &target->library->deps;
    case TargetKind_Targets: return target->targets;
    case TargetKind_UniversalBinary: return &target->universal_binary->bins;
    case TargetKind_UniversalLibrary: return &target->universal_library->libs;
    }
}

static inline void recurse_targets_r(StringSet* seen, Target target, void* user, void(*callback)(void* user, Target target))
{
    char* hash = 0;
    asprintf(&hash, "%s/%s", target_triple_string(target_triple(target)), target.name);
    StringView name = string_view(hash);
    if (string_set_has(seen, name)) {
        free(hash);
        return;
    }
    string_set_add(seen, name);
    callback(user, target);
    Targets const* deps = target_deps(&target);
    usize deps_len = len(deps->entries);
    for (usize i = 0; i < deps_len; i++) {
        recurse_targets_r(seen, deps->entries[i], user, callback);
    }
}

static inline void recurse_targets(Target target, void* user, void(*callback)(void* user, Target target))
{
    StringSet seen = (StringSet){ 0 };
    recurse_targets_r(&seen, target, user, callback);
}

static inline Targets flatten_targets(Target target)
{
    DynTargets dyn = (DynTargets){ 0 };
    recurse_targets(target, &dyn, dyn_targets_add_g);
    return dyn.targets;
}

static inline Strings default_cxx_args(void)
{
    return (Strings){
        "-Wall",
        "-Wextra",
        "-Werror=return-type",
        "-Werror=switch",
        "-Werror",
        "-Wformat=2",
        "-Wimplicit-fallthrough",
        "-Wmissing-declarations",
        "-Wmissing-prototypes",
        "-Wno-c99-designator",
        "-Wno-c99-extensions",
        "-Wno-expansion-to-defined",
        "-Wno-format-pedantic",
        "-Wno-gnu-anonymous-struct",
        "-Wno-gnu-designator",
        "-Wno-gnu-case-range",
        "-Wno-gnu-conditional-omitted-operand",
        "-Wno-gnu-statement-expression",
        "-Wno-gnu-zero-variadic-macro-arguments",
        "-Wno-implicit-const-int-float-conversion",
        "-Wno-invalid-offsetof",
        "-Wno-keyword-macro",
        "-Wno-literal-suffix",
        "-Wno-nested-anon-types",
        "-Wno-unknown-warning-option",
        "-Wno-unused-command-line-argument",
        "-Wno-user-defined-literals",
        "-Wno-gcc-compat",
        "-Wsuggest-override",
        "-fstrict-flex-arrays=2",
        "-Wno-c23-extensions",
        OPTIMIZE_LEVEL,
        "-g3",
        "-gdwarf",
        "-gfull",
        "-gmodules",
        "-glldb",
        "-fno-omit-frame-pointer",
        "-fno-exceptions",
        "-fno-rtti",
        "-fcolor-diagnostics",
    };
}

static inline c_string system_os(void)
{
#if __APPLE__
    return "darwin";
#elif __linux__
    return "linux";
#elif _WIN32
    return "windows";
#error "unknown os"
    return 0;
#endif
}

static inline c_string system_arch(void)
{
#if __aarch64__
    return "arm64";
#elif __x86_64__
    return "x86_64";
#elif __i386__
    return "i386";
#else
#error "unknown architecture"
    return 0;
#endif
}

static inline c_string system_abi(void)
{
#ifdef __APPLE__
    return "apple";
#elif __linux__
    return "gnu";
#elif _WIN32
    return "windows";
#else
#error "unknown abi"
    return 0;
#endif
}

static inline TargetTriple system_target_triple(void)
{
    return (TargetTriple) {
        .arch = system_arch(),
        .abi = system_abi(),
        .os = system_os(),
    };
}

static inline TargetTriple wasm_target_triple(void)
{
    return (TargetTriple) {
        .arch = "wasm32",
        .abi = "unknown",
        .os = "unknown",
    };
}

static inline TargetTriple apple_arm_target_triple(void)
{
    return (TargetTriple) {
        .arch = "arm64",
        .abi = "apple",
        .os = "darwin",
    };
}

static inline TargetTriple apple_x86_target_triple(void)
{
    return (TargetTriple) {
        .arch = "x86_64",
        .abi = "apple",
        .os = "darwin",
    };
}

static inline c_string target_triple_string(TargetTriple triple)
{
    usize len = 0;
    c_string arch = triple.arch ? triple.arch : "unknown";
    c_string abi = triple.abi ? triple.abi : "unknown";
    c_string os = triple.os ? triple.os : "unknown";
    len += strlen(arch) + 1;
    len += strlen(abi) + 1;
    len += strlen(os) + 1;
    char* dest = (char*)calloc(len, 1);
    snprintf(dest, len, "%s-%s-%s", arch, abi, os);
    return dest;
}

static inline TargetTriple target_triple(Target target)
{
    switch (target.kind) {
    case TargetKind_Binary:
        return target.binary->target_triple;
    case TargetKind_SharedLibrary:
    case TargetKind_StaticLibrary:
        return target.library->target_triple;
    case TargetKind_Targets:
        return (TargetTriple){};
    case TargetKind_UniversalLibrary:
    case TargetKind_UniversalBinary:
        return (TargetTriple){
            .arch = "universal",
            .abi = system_abi(),
            .os = system_os(),
        };
    }
}

static inline StringView string_view(c_string string)
{
    return (StringView) {
        .characters = string,
        .size = strlen(string),
    };
}

static inline bool string_view_equal(StringView a, StringView b)
{
    if (a.size != b.size) {
        return 0;
    }
    if (a.size == 0) {
        return 1;
    }
    if (a.characters == b.characters) {
        return 1;
    }
    return *a.characters == *b.characters && memcmp(a.characters + 1, b.characters + 1, a.size - 1) == 0;
}

static inline bool string_set_has(StringSet const* set, StringView item) {
    usize size = set->size;
    for (usize i = 0; i < size; i++) {
        if (string_view_equal(item, set->items[i])) {
            return 1;
        }
    }
    return 0;
}

static inline void string_set_add(StringSet* set, StringView item) {
    if (string_set_has(set, item))
        return;
    usize id = set->size++;
    assert(id < capacity(set->items));
    set->items[id] = item;
}

static inline void dyn_targets_add(DynTargets* dyn, Target target)
{
    usize i = dyn->size++;
    assert(i < capacity(dyn->targets.entries));
    dyn->targets.entries[i] = target;
}

static inline void dyn_targets_add_g(void* targets, Target target)
{
    dyn_targets_add(((DynTargets*)targets), target);
}

#include "./build.def"
int main(void)
{
    setup("build");
    FILE* ninja = fopen("build/build.ninja", "w");
    if (!ninja) {
        perror("bs.h: could not open build/build.ninja");
        return 1;
    }
    emit_ninja(ninja, all_targets);
    fclose(ninja);
}
#endif
