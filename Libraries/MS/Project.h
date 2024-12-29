#pragma once

#include <Ty/Base.h>
#include <Ty/Forward.h>
#include <Ty/Json.h>

namespace MS {

struct Project {
    usize version { 1 };
    f64 tempo { 120.0f };
    u32 sample_rate { 44100 };
    u32 channels { 1 };
    StringView project_name { "no name"sv };

    /// Expecting object with shape:
    /// {
    ///     "version": usize,
    ///     "tempo": f64,
    ///     "sample_rate": f64,
    ///     "channels": f64,
    ///     "res": string[]
    /// }
    static ErrorOr<Project> from_json(Json const& json, JsonObject const& object);

    static void shape(Coder&);
    static Coder shape();
};

}
