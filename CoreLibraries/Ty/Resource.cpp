#include "./Resource.h"
#include "./Bundle.h"

namespace Ty {

Resource::Resource(StringView resolved_path, Bytes bytes)
    : m_resolved_path(resolved_path)
    , m_bytes(bytes)
{
    Bundle::the().add_resource(*this);
}

}
