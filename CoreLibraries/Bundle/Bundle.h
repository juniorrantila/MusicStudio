#pragma once
#include "./Resource.h"

#include <Ty/Forward.h>
#include <Ty/Optional.h>
#include <Ty/SmallVector.h>

namespace Bundle {

struct Bundle {
    Bundle& operator=(Bundle const&) = delete;
    Bundle(Bundle const&) = delete;

    Bundle& operator=(Bundle&&) = delete;
    Bundle(Bundle&&) = delete;

    static Bundle& the(); // NOTE: Generated via make-bundle

    Optional<Resource> resource_with_path(StringView path) const;
    void add_resource(Resource resource);

    View<Resource const> resources() const { return m_resources.view(); }

private:
    constexpr Bundle() = default;

    SmallVector<Resource, 256> m_resources {};
};

static inline Bundle& the() { return Bundle::the(); }

}
