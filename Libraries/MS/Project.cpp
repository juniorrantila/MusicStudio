#include "./Project.h"
#include <Ty/ErrorOr.h>
#include <Ty/Vector.h>

namespace MS {

ErrorOr<Project> Project::from_json(Json const&, JsonObject const& object)
{
    auto version = TRY(TRY(TRY(object.fetch("version"sv).or_throw([]{
        return Error::from_string_literal("could not find version field in project file");
    })).as_number().or_throw([]{
        return Error::from_string_literal("version field was not a number");
    })).as_usize().or_throw([] {
        return Error::from_string_literal("expected version to be usize");
    }));
    if (version < 1) {
        return Error::from_string_literal("unsupported project version");
    }

    auto tempo = TRY(object.fetch("tempo"sv).or_else(120.0).as_number().or_throw([]{
        return Error::from_string_literal("expected tempo to be numeric");
    }));
    if (tempo < 0.0) {
        return Error::from_string_literal("expected tempo to be positive");
    }

    auto sample_rate = TRY(object.fetch("sample_rate"sv).or_else(44100.0).as_number().or_throw([]{
        return Error::from_string_literal("expected sample_rate to be numeric");
    }));
    if (sample_rate < 0.0) {
        return Error::from_string_literal("expected channels to be positive");
    }

    auto channels = TRY(object.fetch("channels"sv).or_else(1.0).as_number().or_throw([]{
        return Error::from_string_literal("expected channels to be numeric");
    }));
    if (channels < 0.0) {
        return Error::from_string_literal("expected channels to be positive");
    }

    return Project {
        .version = version,
        .tempo = tempo,
        .sample_rate = (u32)sample_rate,
        .channels = (u32)channels,
    };
}

}
