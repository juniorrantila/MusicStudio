#include "./Project.h"
#include <Ty/ErrorOr.h>
#include <Ty/Vector.h>
#include <Ty/Coder.h>

namespace MS {

ErrorOr<Project> Project::from_json(Json const& json, JsonObject const& object)
{
    auto coder = shape();

    auto project = Project();
    TRY(json_decode_into(json, object, coder.fields().view(), &project));
    if (project.version != 1) {
        return Error::from_string_literal("unsupported project version");
    }

    return project;
}

void Project::shape(Coder& coder)
{
    coder.required_field(&Project::version, "version");
    coder.optional_field(&Project::tempo, "tempo");
    coder.optional_field(&Project::sample_rate, "sample_rate");
    coder.optional_field(&Project::channels, "channels");
    coder.optional_field(&Project::project_name, "project_name");
}

Coder Project::shape()
{
    auto coder = Coder();
    shape(coder);
    return coder;
}

}
