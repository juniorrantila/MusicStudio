#pragma once
#include "./Resource.h"
#include "Ty/LinearMap.h"

#include <Ty/Forward.h>
#include <Ty/Optional.h>
#include <Ty/Vector.h>
#include <Ty/StringBuffer.h>

namespace FS {

struct Bundle {
    constexpr Bundle() = default;
    Bundle& operator=(Bundle const&) = delete;
    Bundle(Bundle const&) = delete;

    Bundle(Bundle&& other) = default;
    Bundle& operator=(Bundle&&) = default;

    static ErrorOr<Bundle> create_from_bytes(Bytes);
    static ErrorOr<Bundle> create_from_path(StringView);

    ErrorOr<void> mount_bytes(Bytes, StringView mount_point);
    ErrorOr<void> mount(StringView path, StringView mount_point);

    Optional<ResourceView> open(StringView path);
    void unsafe_add_resource(ResourceView resource);

    View<ResourceView const> resources() const { return m_unsafe_resources.view(); }
    ErrorOr<Bytes> bytes();

    template <typename T>
    Bundle add_pack(T pack) && {
        pack.add_to_bundle(*this);
        return move(*this);
    }

    template <typename T>
    Bundle& add_pack(T pack) & {
        pack.add_to_bundle(*this);
        return *this;
    }

private:
    ErrorOr<void> saturate_zip_buffer();
    ErrorOr<ResourceView> saturate_resource_file(StringView fs_path, StringView mount_point);

    Vector<ResourceView> m_unsafe_resources {};

    using MountPoint = StringBuffer;
    LinearMap<MountPoint, Resource> m_cached_resources {};
    LinearMap<MountPoint, StringBuffer> m_file_mounts {};
    LinearMap<MountPoint, StringBuffer> m_directory_mounts {};
    LinearMap<MountPoint, StringBuffer> m_zip_mounts {};

    StringBuffer m_combined_zip_buffer {};
};

}
