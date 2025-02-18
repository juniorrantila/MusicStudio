#include "./Bundle.h"
#include "Resource.h"

#include <Core/MappedFile.h>
#include <Core/File.h>
#include <Ty/Defer.h>
#include <Ty/StringBuffer.h>
#include <Ty/System.h>
#include <unistd.h>

namespace FS {

Optional<ResourceView> Bundle::open(StringView path) const
{
    auto mount_path = MUST(path.resolve_path());
    for (auto const& resource : resources()) {
        if (mount_path.view() == resource.resolved_path())
            return resource;
    }
    if (auto mount_point = m_file_mounts.find(mount_path.view())) {
        auto const& fs_path = m_file_mounts[*mount_point];
        auto resource = saturate_resource_file(fs_path.view(), mount_path.view());
        if (resource.is_error()) {
            return {};
        }
        return resource.release_value();
    }
    return {};
}

ErrorOr<ResourceView> Bundle::saturate_resource_file(StringView fs_path, StringView mount_path) const
{
    auto file = TRY(Core::MappedFile::open(fs_path, Core::MappedFile::ModeRead));
    auto id = TRY(m_cached_resources.set(
        TRY(StringBuffer::create_fill(fs_path)),
        Resource::create_with_resolved_path(
            TRY(StringBuffer::create_fill(mount_path)),
            TRY(StringBuffer::create_fill(file.view()))
        )
    ));
    TRY(m_unsafe_resources.append(m_cached_resources[id].resource_view()));
    return m_cached_resources[id].resource_view();
}

void Bundle::unsafe_add_resource(ResourceView resource)
{
    MUST(m_unsafe_resources.append(resource));
}

ErrorOr<void> Bundle::mount_bytes(Bytes, StringView mount_point)
{
    (void)mount_point;
    return Error::unimplemented();
}

ErrorOr<void> Bundle::mount(StringView file, StringView mount_point)
{
    if (TRY(Core::File::is_directory(file))) {
        TRY(m_directory_mounts.set(
            TRY(StringBuffer::create_fill(mount_point, "\0"sv)),
            TRY(StringBuffer::create_fill(file, "\0"sv))
        ));
        return {};
    }

    TRY(m_file_mounts.set(
        TRY(StringBuffer::create_fill(mount_point, "\0"sv)),
        TRY(StringBuffer::create_fill(file, "\0"sv))
    ));

    return {};
}

}
