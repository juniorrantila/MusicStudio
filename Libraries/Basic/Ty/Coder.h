#pragma once
#include "./Base.h"
#include "./StringView.h"
#include "./SmallVector.h"

#include <Basic/Bits.h>

namespace Ty {

enum class CoderFieldType {
    Bool,
    I8,
    I16,
    I32,
    I64,
    U8,
    U16,
    U32,
    U64,
    F32,
    F64,
    String,
    BeginObject,
    EndObject,
};

struct CoderField {
    StringView name;
    uptr offset;
    CoderFieldType type;
    bool required;
};

struct Coder {
    using enum CoderFieldType;

    template <typename T> void required_field(bool T::* field, StringView name) { add(field, name, Bool, true); }
    template <typename T> void optional_field(bool T::* field, StringView name) { add(field, name, Bool, false); }

    template <typename T> void required_field(i8  T::* field, StringView name) { add(field, name, I8,  true);  }
    template <typename T> void optional_field(i8  T::* field, StringView name) { add(field, name, I8,  false); }
    template <typename T> void required_field(i16 T::* field, StringView name) { add(field, name, I16, true);  } 
    template <typename T> void optional_field(i16 T::* field, StringView name) { add(field, name, I16, false); }
    template <typename T> void required_field(i32 T::* field, StringView name) { add(field, name, I32, true);  } 
    template <typename T> void optional_field(i32 T::* field, StringView name) { add(field, name, I32, false); }
    template <typename T> void required_field(i64 T::* field, StringView name) { add(field, name, I64, true);  } 
    template <typename T> void optional_field(i64 T::* field, StringView name) { add(field, name, I64, false); }

    template <typename T> void required_field(u8  T::* field, StringView name) { add(field, name, U8,  true);  }
    template <typename T> void optional_field(u8  T::* field, StringView name) { add(field, name, U8,  false); }
    template <typename T> void required_field(u16 T::* field, StringView name) { add(field, name, U16, true);  } 
    template <typename T> void optional_field(u16 T::* field, StringView name) { add(field, name, U16, false); }
    template <typename T> void required_field(u32 T::* field, StringView name) { add(field, name, U32, true);  } 
    template <typename T> void optional_field(u32 T::* field, StringView name) { add(field, name, U32, false); }
    template <typename T> void required_field(u64 T::* field, StringView name) { add(field, name, U64, true);  } 
    template <typename T> void optional_field(u64 T::* field, StringView name) { add(field, name, U64, false); }

    template <typename T> void required_field(f32 T::* field, StringView name) { add(field, name, F32, true);  } 
    template <typename T> void optional_field(f32 T::* field, StringView name) { add(field, name, F32, false); }
    template <typename T> void required_field(f64 T::* field, StringView name) { add(field, name, F64, true);  } 
    template <typename T> void optional_field(f64 T::* field, StringView name) { add(field, name, F64, false); }

    template <typename T> void required_field(StringView T::* field, StringView name) { add(field, name, String, true);  } 
    template <typename T> void optional_field(StringView T::* field, StringView name) { add(field, name, String, false); }

    template <typename T, typename FT>
    void required_object(FT* o, FT T::* field, StringView name)
    {
        add(field, name, BeginObject, true);

        o->code(*this);

        add(field, name, EndObject, true);
    }

    template <typename T, typename FT>
    void optional_object(FT* o, FT T::* field, StringView name)
    {
        add(field, name, BeginObject, false);

        o->code(*this);

        add(field, name, EndObject, false);
    }

    SmallVector<CoderField> const& fields() const { return m_fields; }

private:
    template <typename T, typename TF>
    void add(TF T::* field, StringView name, CoderFieldType type, bool required)
    {
        m_fields.must_append({
            .name = name,
            .offset = ty_bit_cast<uptr>(field),
            .type = type,
            .required = required,
        });
    }

    SmallVector<CoderField> m_fields {};
};

}
