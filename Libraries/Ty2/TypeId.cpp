#include "./TypeId.h"

static c_string type_names[65536];

bool Ty2::type_register(u16 tag, c_string name)
{
    type_names[tag] = name;
    return true;
}

C_API c_string ty_type_name(u16 tag)
{
    return type_names[tag];
}
