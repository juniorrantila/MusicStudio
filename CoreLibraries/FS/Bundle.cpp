#include "./Bundle.h"
#include <Ty/StringBuffer.h>

namespace FS {

Optional<Resource> Bundle::resource_with_path(StringView path) const
{
    auto resolved_path = MUST(path.resolve_path());
    for (auto const& resource : m_resources) {
        if (resolved_path.view() == resource.resolved_path())
            return resource;
    }
    return {};
}

void Bundle::add_resource(Resource resource)
{
    MUST(m_resources.append(resource));
}

}
