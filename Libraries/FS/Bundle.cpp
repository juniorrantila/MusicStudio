#include "./Bundle.h"
#include "Resource.h"

#include <Core/MappedFile.h>
#include <Core/File.h>
#include <Ty/Defer.h>
#include <Ty/StringBuffer.h>
#include <Ty/System.h>
#include <Zip/Zip.h>
#include <unistd.h>

namespace FS {

Optional<ResourceView> Bundle::open(StringView path)
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

ErrorOr<ResourceView> Bundle::saturate_resource_file(StringView fs_path, StringView mount_path)
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

static ErrorOr<bool> is_zip(StringView path);
ErrorOr<void> Bundle::mount(StringView file, StringView mount_point)
{
    if (TRY(Core::File::is_directory(file))) {
        TRY(m_directory_mounts.set(
            TRY(StringBuffer::create_fill(mount_point, "\0"sv)),
            TRY(StringBuffer::create_fill(file, "\0"sv))
        ));
        return {};
    }

    if (TRY(is_zip(file))) {
        TRY(m_zip_mounts.set(
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

ErrorOr<Bytes> Bundle::bytes()
{
    TRY(saturate_zip_buffer());
    return m_combined_zip_buffer.view().as_bytes();
}

// FIXME: Modify zip library to operate on buffers.
ErrorOr<void> Bundle::saturate_zip_buffer()
{
    m_combined_zip_buffer.clear();
    char tmp_filename[] = "/tmp/XXXXXX";
    if (mkstemp(tmp_filename) < 0) {
        return Error::from_errno();
    }

    int error = 0;
    zip_t* zip = zip_openwitherror(tmp_filename, 0, 'w', &error);
    if (!zip) {
        c_string message = zip_strerror(error);
        return Error::from_string_literal(message);
    }
    Defer close_zip = [&] {
        zip_close(zip);
    };

    for (usize raw_entry = 0; raw_entry < m_file_mounts.size(); raw_entry++) {
        auto id = Id<StringBuffer>(raw_entry);
        auto file_path = m_file_mounts.keys()[raw_entry].view().data();
        auto mount_name = m_file_mounts[id].view().data();
        if (int res = zip_entry_open(zip, file_path); res < 0) {
            c_string message = zip_strerror(res);
            return Error::from_string_literal(message);
        }
        Defer close_entry = [&] {
            zip_entry_close(zip);
        };

        if (int res = zip_entry_fwrite(zip, mount_name); res < 0) {
            c_string message = zip_strerror(res);
            return Error::from_string_literal(message);
        }
    }
    close_zip.run();

    auto file = TRY(Core::MappedFile::open(tmp_filename));
    TRY(m_combined_zip_buffer.expand_if_needed_for_write(file.size()));
    TRY(m_combined_zip_buffer.write(file.view()));
    TRY(System::unlink(tmp_filename));

    return {};
}

static ErrorOr<bool> is_zip(StringView path)
{
    auto path_buf = TRY(StringBuffer::create_fill(path, "\0"sv));
    int error = 0;
    zip_close(zip_openwitherror(path_buf.data(), 0, 'w', &error));

    // FIXME: Technically speaking it could be a zip file error.
    return error != 0;
}

}
