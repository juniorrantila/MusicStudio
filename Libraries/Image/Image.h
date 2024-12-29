#pragma once
#include <Ty/Forward.h>
#include <Ty/Vector.h>

struct Image {
    static ErrorOr<Image> load_from_bytes(Bytes);

    usize channels() const { return m_channels; }
    usize width() const { return m_width; }
    usize height() const { return m_height; }

    View<f32 const> data() const { return m_image.view(); }
    View<f32> data() { return m_image.view(); }

    f32 const& operator[](usize i) const { return m_image[i]; }
    f32& operator[](usize i) { return m_image[i]; }

private:
    constexpr Image(Vector<f32> image, usize width, usize height, usize channels)
        : m_image(move(image))
        , m_width(width)
        , m_height(height)
        , m_channels(channels)
    {
    }

    Vector<f32> m_image {};
    usize m_width { 0 };
    usize m_height { 0 };
    usize m_channels { 0 };
};
