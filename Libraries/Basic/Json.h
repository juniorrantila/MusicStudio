#include "./Base.h"
TY_CPP_ONLY();

#include "./Span.h"
#include "./Maybe.h"

struct Coder2;

namespace Ty2 {

template <typename T>
inline constexpr bool IsFunction = __is_function(T);

template <typename T>
inline constexpr bool IsPointer = __is_pointer(T);

template <typename T>
struct remove_pointer {
    using Type = T;
};

template <typename T>
struct remove_pointer<T*> {
    using Type = T;
};

template <typename T>
using RemovePointer = typename remove_pointer<T>::Type;

template <typename T>
inline constexpr bool IsFunctionPointer = IsPointer<T> && IsFunction<RemovePointer<T>>;

template<typename T>
auto declval() -> T;

template <typename T, typename U>
concept IsSame = __is_same(T, U);

template <typename T>
concept IsEnum = __is_enum(T);

template<typename T, typename Out, typename... Args>
inline constexpr bool IsCallableWithArguments = requires(T t) {
    {
        t(declval<Args>()...)
    } -> IsSame<Out>;
};

}

template <typename T, typename C = Coder2>
concept HasCoder = requires(c_string name, T* value, C coder)
{
    { T::code(&coder, value) } -> Ty2::IsSame<void>;
};

template <typename T>
concept IsVariant = requires {
    { Ty2::IsEnum<typename T::Kind::Inner> };
    { T::Kind::__Count };
};

enum class CoderKind : u8 {
    Bool,
    U8,
    I8,
    U16,
    I16,
    U32,
    I32,
    U64,
    I64,
    F32,
    F64,
    CString,

    Object,

    BeginArray,
    EndArray,
    BeginObject,
    EndObject,
    BeginVariant,
    EndVariant,
};

enum class CoderRequiredField : bool {
    No = false,
    Yes = true,
};

struct CoderMeta {
    c_string name;
    void* location;
    void (*code)(Coder2* coder, void* location);
    CoderKind kind;
    bool required;
};

template <typename T> struct CoderForKind;
template <> struct CoderForKind<u8>  { static constexpr CoderKind kind = CoderKind::U8; };
template <> struct CoderForKind<u16> { static constexpr CoderKind kind = CoderKind::U16; };
template <> struct CoderForKind<u32> { static constexpr CoderKind kind = CoderKind::U32; };
template <> struct CoderForKind<u64> { static constexpr CoderKind kind = CoderKind::U64; };
template <> struct CoderForKind<i8>  { static constexpr CoderKind kind = CoderKind::I8; };
template <> struct CoderForKind<i16> { static constexpr CoderKind kind = CoderKind::I16; };
template <> struct CoderForKind<i32> { static constexpr CoderKind kind = CoderKind::I32; };
template <> struct CoderForKind<i64> { static constexpr CoderKind kind = CoderKind::I64; };
template <> struct CoderForKind<f32> { static constexpr CoderKind kind = CoderKind::F32; };
template <> struct CoderForKind<f64> { static constexpr CoderKind kind = CoderKind::F64; };
template <> struct CoderForKind<c_string> { static constexpr CoderKind kind = CoderKind::CString; };

template <typename T> struct DefaultCodable { static constexpr bool value = false; };
template <> struct DefaultCodable<u8>  { static constexpr bool value = true; };
template <> struct DefaultCodable<u16> { static constexpr bool value = true; };
template <> struct DefaultCodable<u32> { static constexpr bool value = true; };
template <> struct DefaultCodable<u64> { static constexpr bool value = true; };
template <> struct DefaultCodable<i8>  { static constexpr bool value = true; };
template <> struct DefaultCodable<i16> { static constexpr bool value = true; };
template <> struct DefaultCodable<i32> { static constexpr bool value = true; };
template <> struct DefaultCodable<i64> { static constexpr bool value = true; };
template <> struct DefaultCodable<f32> { static constexpr bool value = true; };
template <> struct DefaultCodable<f64> { static constexpr bool value = true; };
template <> struct DefaultCodable<c_string> { static constexpr bool value = true; };

template <typename T>
concept IsDefaultCodable = DefaultCodable<T>::value;

struct Coder2 {
    template <typename T>
        requires HasCoder<T>
    static Coder2 from(T* value)
    {
        Coder2 coder;
        T::code(&coder, value);
        return coder;
    }

    Span<CoderMeta const> description() const { return Span(m_meta, m_meta_count); }

    template <typename T>
        requires HasCoder<T>
    void field(c_string name, T* value)
    {
        add((CoderMeta){
            .name = name,
            .location = value,
            .code = nullptr,
            .kind = CoderKind::BeginObject,
            .required = true,
        });

        T::code(this, value);

        add((CoderMeta){
            .name = name,
            .location = value,
            .code = nullptr,
            .kind = CoderKind::EndObject,
            .required = true,
        });
    }

    template <typename T>
        requires HasCoder<T>
    void field(c_string name, Span<T>* value)
    {
        add((CoderMeta){
            .name = name,
            .location = value,
            .code = nullptr,
            .kind = CoderKind::BeginArray,
            .required = true,
        });

        T::code(this, nullptr);

        add((CoderMeta){
            .name = name,
            .location = value,
            .code = nullptr,
            .kind = CoderKind::EndArray,
            .required = true,
        });
    }

    template <typename T>
        requires IsDefaultCodable<T>
    void field(c_string name, T* location)
    {
        add((CoderMeta){
            .name = name,
            .location = location,
            .code = nullptr,
            .kind = CoderForKind<T>::kind,
            .required = true,
        });
    }

    template <typename T>
        requires HasCoder<T>
    void field(c_string name, Maybe<T>* value)
    {
        add((CoderMeta){
            .name = name,
            .location = value,
            .code = nullptr,
            .kind = CoderKind::BeginObject,
            .required = false,
        });

        T::code(this, nullptr);

        add((CoderMeta){
            .name = name,
            .location = value,
            .code = nullptr,
            .kind = CoderKind::EndObject,
            .required = false,
        });
    }

    template <typename T>
        requires HasCoder<T>
    void field(c_string name, Maybe<Span<T>>* value)
    {
        add((CoderMeta){
            .name = name,
            .location = value,
            .code = nullptr,
            .kind = CoderKind::BeginArray,
            .required = false,
        });

        T::code(this, nullptr);

        add((CoderMeta){
            .name = name,
            .location = value,
            .code = nullptr,
            .kind = CoderKind::EndArray,
            .required = false,
        });
    }

    template <typename T>
        requires IsDefaultCodable<T>
    void field(c_string name, Maybe<Span<T>>* value)
    {
        add((CoderMeta){
            .name = name,
            .location = value,
            .code = nullptr,
            .kind = CoderKind::BeginArray,
            .required = false,
        });

        add((CoderMeta){
            .name = name,
            .location = nullptr,
            .code = nullptr,
            .kind = CoderForKind<T>::kind,
            .required = true,
        });

        add((CoderMeta){
            .name = name,
            .location = value,
            .code = nullptr,
            .kind = CoderKind::EndArray,
            .required = false,
        });
    }

    template <typename T>
        requires IsVariant<T>
    void variant(c_string tag, T* value, void(*callback)(Coder2*, void*))
    {
        static auto coder = [](Coder2*, void*){
            static c_string options[T::Kind::__Count];
            for (u64 i = 0; i < (u64)T::Kind::__Count; i++) {
                typename T::Kind self = (typename T::Kind::Inner)i;
                options[i] = self.name();
            }
        };

        add((CoderMeta){
            .name = tag,
            .location = &value->kind,
            .code = coder,
            .kind = CoderKind::BeginVariant,
            .required = true,
        });

        add((CoderMeta){
            .name = nullptr,
            .location = value,
            .code = callback,
            .kind = CoderKind::Object,
            .required = true,
        });

        add((CoderMeta){
            .name = tag,
            .location = value,
            .code = nullptr,
            .kind = CoderKind::EndVariant,
            .required = true,
        });
    }

private:
    CoderMeta m_meta[1024] = {};
    u32 m_meta_count = 0;

    void add(CoderMeta meta)
    {
        VERIFY(m_meta_count < ty_array_size(m_meta));
        m_meta[m_meta_count++] = meta;
    }

    template <typename T>
        requires requires {
            { T::Inner } -> Ty2::IsEnum;
            { T::__Count };
        } and requires(T field) {
            { field.name() } -> Ty2::IsSame<c_string>;
        }
    static void enum_coder(T* location)
    {
        (void)location;
        static c_string options[T::__Count];
        for (u64 i = 0; i < (u64)T::__Count; i++) {
            T self = (typename T::Inner)i;
            options[i] = self.name();
        }
        // coder->enum_field(kind, options, Self::__Count);
    }
};
