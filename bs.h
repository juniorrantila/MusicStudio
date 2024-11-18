#pragma once
#include <stdlib.h>
#include <libgen.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/stat.h>

#define MAX_ENTRIES 128

typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef signed long i64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long u64;

static_assert(sizeof(i8) == 1);
static_assert(sizeof(i16) == 2);
static_assert(sizeof(i32) == 4);
static_assert(sizeof(i64) == 8);

static_assert(sizeof(u8) == 1);
static_assert(sizeof(u16) == 2);
static_assert(sizeof(u32) == 4);
static_assert(sizeof(u64) == 8);

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
    char const* characters { nullptr };
    usize size { 0 };
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
    TargetKind_Library,
    TargetKind_Targets,
} TargetKind;

struct BinaryArgs;
struct LibraryArgs;
struct Targets;
typedef struct {
    c_string name;
    c_string file;
    c_string base_dir;
    union {
        struct BinaryArgs* binary;
        struct LibraryArgs* library;
        struct Targets* targets;
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

static inline Strings default_cpp_args(void);
static inline Strings default_c_args(void);
static inline Strings default_objc_args(void);
static inline Strings default_objcpp_args(void);

template <typename T, usize Count>
static inline usize len(T const (& items)[Count]);

template <typename T, usize Count>
static inline usize capacity(T const (& items)[Count]);

template <typename T, usize Count, usize Count2>
static inline void cat(T (&items)[Count], T const (& other)[Count2]);

template <typename F>
static inline auto target(F callback, c_string file = __builtin_FILE());

static inline Target cpp_binary(c_string name, BinaryArgs args, c_string file = __builtin_FILE());
static inline Target cpp_library(c_string name, LibraryArgs args, c_string file = __builtin_FILE());
static inline Target c_library(c_string name, LibraryArgs args, c_string file = __builtin_FILE());
static inline Target objc_library(c_string name, LibraryArgs args, c_string file = __builtin_FILE());
static inline Target objcpp_library(c_string name, LibraryArgs args, c_string file = __builtin_FILE());
static inline Target cpp_bundle_library(c_string name, LibraryArgs args, c_string file = __builtin_FILE());

static inline void emit_ninja(FILE* output, Target target);

static inline void recurse_targets(Target target, void* user, void(*callback)(void* user, Target target));

static inline Targets flatten_targets(Target target);

static inline c_string system_os(void);
static inline c_string system_arch(void);
static inline c_string system_abi(void);

static inline TargetTriple system_target_triple(void);
static inline TargetTriple wasm_target_triple(void);
static inline TargetTriple dynamic_target_tripple(void);
static inline c_string target_triple_string(TargetTriple triple);

static inline void setup(c_string, c_string file = __builtin_FILE());

static inline void dyn_targets_add(DynTargets* targets, Target target);
static inline void dyn_targets_add_g(void* targets, Target target);

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

static inline Targets all_targets_deps;
static inline usize all_targets_count = 0;
static inline Target all_targets = {
    .name = "all",
    .file = "",
    .base_dir = "",
    .targets = &all_targets_deps,
    .kind = TargetKind_Targets,
};
static inline Target cpp_binary(c_string name, BinaryArgs args, c_string file)
{
    c_string base_dir = strdup(dirname(strdup(file)));
    BinaryArgs* res = (BinaryArgs*)malloc(sizeof(args));
    *res = args;
    res->compile_flags = default_cpp_args();
    res->target_triple = system_target_triple();
    cat(res->compile_flags.entries, args.compile_flags.entries);
    Target target = {
        .name = name,
        .file = file,
        .base_dir = base_dir,
        .binary = res,
        .kind = TargetKind_Binary,
    };
    all_targets_deps.entries[all_targets_count++] = target;
    return target;
}

static inline Target cpp_library(c_string name, LibraryArgs args, c_string file)
{
    c_string base_dir = strdup(dirname(strdup(file)));
    LibraryArgs* res = (LibraryArgs*)malloc(sizeof(args));
    *res = args;
    res->compile_flags = default_cpp_args();
    cat(res->compile_flags.entries, args.compile_flags.entries);
    Target target = {
        .name = name,
        .file = file,
        .base_dir = base_dir,
        .library = res,
        .kind = TargetKind_Library,
    };
    all_targets_deps.entries[all_targets_count++] = target;
    return target;
}

static inline Target c_library(c_string name, LibraryArgs args, c_string file)
{
    c_string base_dir = strdup(dirname(strdup(file)));
    LibraryArgs* res = (LibraryArgs*)malloc(sizeof(args));
    *res = args;
    res->compile_flags = default_c_args();
    cat(res->compile_flags.entries, args.compile_flags.entries);
    Target target = {
        .name = name,
        .file = file,
        .base_dir = base_dir,
        .library = res,
        .kind = TargetKind_Library,
    };
    all_targets_deps.entries[all_targets_count++] = target;
    return target;
}

static inline Target objc_library(c_string name, LibraryArgs args, c_string file)
{
    c_string base_dir = strdup(dirname(strdup(file)));
    LibraryArgs* res = (LibraryArgs*)malloc(sizeof(args));
    *res = args;
    res->compile_flags = default_objc_args();
    cat(res->compile_flags.entries, args.compile_flags.entries);
    Target target = {
        .name = name,
        .file = file,
        .base_dir = base_dir,
        .library = res,
        .kind = TargetKind_Library,
    };
    all_targets_deps.entries[all_targets_count++] = target;
    return target;
}

static inline Target objcpp_library(c_string name, LibraryArgs args, c_string file)
{
    c_string base_dir = strdup(dirname(strdup(file)));
    LibraryArgs* res = (LibraryArgs*)malloc(sizeof(args));
    *res = args;
    res->compile_flags = default_objcpp_args();
    cat(res->compile_flags.entries, args.compile_flags.entries);
    Target target = {
        .name = name,
        .file = file,
        .base_dir = base_dir,
        .library = res,
        .kind = TargetKind_Library,
    };
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
static TargetRule ninja_rule(TargetRule rule)
{
    all_rules[all_rules_count++] = rule;
    return rule;
}

static inline TargetRule cxx_rule = ninja_rule({
    .name = "cxx",
    .command = "ccache clang++ -target $target $args -MD -MQ $out -MF $out.d -o $out -c $in",
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

typedef struct Language {
    c_string name;
    c_string extension;
} Language;

static inline c_string language_from_filename(c_string name)
{
    Language languages[] = {
        { .name = "C++", .extension = ".cpp" },
        { .name = "C", .extension = ".c" },
        { .name = "Objective-C++", .extension = ".mm" },
        { .name = "Objective-C", .extension = ".m" },
    };
    usize name_len = strlen(name);
    for (usize i = 0; i < capacity(languages); i++) {
        Language language = languages[i];
        usize extension_len = strlen(language.extension);
        if (name_len < extension_len) {
            continue;
        }
        c_string ext = name + name_len - extension_len;
        if (strcmp(ext, language.extension) == 0) {
            return language.name;
        }
    }
    return nullptr;
}

static inline TargetRule merge_object_rule = ninja_rule({
    .name = "merge-object",
    .command = "$ld -r $link_args -o $out $in",
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
    },
});

static inline TargetRule binary_link_rule = ninja_rule({
    .name = "link-binary",
    .command = "clang++ -target $target -o $out $in $link_args",
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

static inline TargetRule compdb_rule = ninja_rule({
    .name = "compdb",
    .command = "ninja -t compdb > compile_commands.json",
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
        }
    }
});

static inline c_string g_root_file;
static inline c_string g_build_dir;
static inline void setup(c_string build_dir, c_string file)
{
    g_root_file = file;
    g_build_dir = build_dir;
    mkdir(build_dir, 0777);
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

    fprintf(output, "build %s/%s: link-binary", triple, name);
    for (usize i = 0; i < srcs_len; i++) {
        c_string src = binary->srcs.entries[i];
        fprintf(output, " %s/%s/%s.o", triple, base_dir, src);
    }
    for (usize i = 1; i < deps_len; i++) {
        Target const* dep = &deps.entries[i];
        fprintf(output, " %s/%s.o", triple, dep->name);
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
        c_string src = binary->srcs.entries[i];
        fprintf(output, "build %s/%s/%s.o: cxx ../%s/%s\n", triple, base_dir, src, base_dir, src);
        fprintf(output, "    language = %s\n", language_from_filename(src));
        fprintf(output, "    target = %s\n", triple);
        fprintf(output, "    args =");
        for (usize arg = 0; arg < args_len; arg++) {
            fprintf(output, " %s", args->entries[arg]);
        }
        for (usize dep_index = 1; dep_index < deps_len; dep_index++) {
            Target const* dep = &deps.entries[dep_index];
            if (dep->kind == TargetKind_Library) {
                LibraryArgs const* library = dep->library;
                c_string ns = library->header_namespace;
                fprintf(output, " -Ins/%s/h", ns);
            }
        }
        fprintf(output, "\n");
        fprintf(output, "\n");
    }
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

static inline void emit_ninja_build_library(FILE* output, Target const* target)
{
    LibraryArgs* library = target->library;
    c_string triple = target_triple_string(library->target_triple);
    c_string base_dir = target->base_dir;
    usize srcs_len = len(library->srcs.entries);
    usize headers_len = len(library->exported_headers.entries);
    c_string name = target->name;

    char* dir = 0;
    asprintf(&dir, "%s/ns/%s/h/%s", g_build_dir, name, name);
    mkdir_p(dir, 0777);
    for (usize i = 0; i < headers_len; i++) {
        c_string header = library->exported_headers.entries[i];
        char* out = 0;
        asprintf(&out, "%s/%s", base_dir, header);
        c_string header_path = realpath(out, 0);
        char* output_path = 0;
        asprintf(&output_path, "%s/%s", dir, header);
        symlink(header_path, output_path);
    }
    
    fprintf(output, "build %s/%s.o: merge-object ", triple, name);
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
    fprintf(output, "    ld = ld\n");
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
        fprintf(output, "build %s/%s/%s.o: cxx ../%s/%s\n", triple, base_dir, src, base_dir, src);
        fprintf(output, "    language = %s\n", language_from_filename(src));
        fprintf(output, "    target = %s\n", triple);
        fprintf(output, "    args =");
        for (usize arg = 0; arg < args_len; arg++) {
            fprintf(output, " %s", args->entries[arg]);
        }
        for (usize dep_index = 1; dep_index < deps_len; dep_index++) {
            Target const* dep = &deps.entries[dep_index];
            if (dep->kind == TargetKind_Library) {
                LibraryArgs const* library = dep->library;
                c_string ns = library->header_namespace;
                fprintf(output, " -Ins/%s/h", ns);
            }
        }
        fprintf(output, "\n");
        fprintf(output, "\n");
    }
}

static inline c_string extra_target_files[MAX_ENTRIES];

static inline void emit_ninja(FILE* output, Target target)
{
    fprintf(output, "ninja_required_version = 1.8.2\n\n");

    for (usize i = 0; i < all_rules_count; i++) {
        emit_ninja_rule(output, &all_rules[i]);
    }

    fprintf(output, "build compile_commands.json: compdb\n\n");

    Targets targets = flatten_targets(target);
    usize targets_len = len(targets.entries);
    fprintf(output, "build build.ninja: reconfigure ../%s", g_root_file);
    fprintf(output, " ../%s", __FILE__);
    usize extra_target_files_len = len(extra_target_files);
    for (usize i = 0; i < extra_target_files_len; i++) {
        fprintf(output, " ../%s", extra_target_files[i]);
    }
    for (usize i = 0; i < targets_len; i++) {
        Target const* target = &targets.entries[i];
        if (target->file[0] == '\0')
            continue;
        fprintf(output, " ../%s", target->file);
    }
    fprintf(output, "\n    configure = %s\n\n", g_root_file);

    for (usize i = 0; i < targets_len; i++) {
        Target const* target = &targets.entries[i];
        switch (target->kind) {
        case TargetKind_Binary:
            emit_ninja_build_binary(output, target);
            break;
        case TargetKind_Library:
            emit_ninja_build_library(output, target);
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
    case TargetKind_Library: return &target->library->deps;
    case TargetKind_Targets: return target->targets;
    }
}

static inline void recurse_targets_r(StringSet* seen, Target target, void* user, void(*callback)(void* user, Target target))
{
    StringView name = string_view(target.name);
    if (string_set_has(seen, name)) {
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
        "-Wsuggest-override",
        "-fstrict-flex-arrays=2",
        "-Wno-c23-extensions",
        "-g3",
        "-gdwarf",
        "-gfull",
        "-gmodules",
        "-glldb",
        "-fno-omit-frame-pointer",
        "-fcolor-diagnostics",
    };
}

static inline Strings default_cpp_args(void)
{
    Strings args = default_cxx_args();
    args.entries[len(args.entries)] = "-std=c++20";
    return args;
}

static inline Strings default_c_args(void)
{
    Strings args = default_cxx_args();
    args.entries[len(args.entries)] = "-std=c23";
    args.entries[len(args.entries)] = "-xc";
    return args;
}

static inline Strings default_objc_args(void)
{
    Strings args = default_c_args();
    args.entries[len(args.entries)] = "-xobjective-c";
    return args;
}

static inline Strings default_objcpp_args(void)
{
    Strings args = default_cpp_args();
    args.entries[len(args.entries)] = "-xobjective-c++";
    return args;
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
        .abi = nullptr,
        .os = nullptr,
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
