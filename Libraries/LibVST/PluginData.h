#pragma once
#include "./AEffect.h"

namespace Vst {

#if 0
struct PluginData {
    constexpr bool plugin_name_is_set() const
    {
        return plugin_name[0] != 0;
    }

    constexpr bool author_name_is_set() const
    {
        return author_name[0] != 0;
    }

    constexpr bool product_name_is_set() const
    {
        return product_name[0] != 0;
    }

    constexpr bool name_of_preset_with_id_name_is_allocated(i32 id) const
    {
        return preset_names[id] != nullptr;
    }

    void allocate_space_for_name_of_preset_with_id(i32 id)
    {
        preset_names[id] = (char*)__builtin_malloc(Vst::MAX_NAME_LENGTH);
        for (auto i = 0; i < Vst::MAX_NAME_LENGTH; i++)
            preset_names[id][i] = '\0';
    }

    constexpr bool name_of_preset_with_id_has_value(i32 id) const
    {
        return preset_names[id] && preset_names[id][0] != '\0';
    }

    char plugin_name[Vst::MAX_NAME_LENGTH] {};
    char author_name[Vst::MAX_NAME_LENGTH] {};
    char product_name[Vst::MAX_NAME_LENGTH] {};
    char** preset_names;
};
#endif

}
