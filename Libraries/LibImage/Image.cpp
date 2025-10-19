#include "./Image.h"

#include <LibTy/ErrorOr.h>

#include <Basic/Defer.h>
#include <Basic/Bytes.h>

#include <stb/image.h>

ErrorOr<Image> Image::load_from_bytes(Bytes bytes)
{
    int width = 0;
    int height = 0;
    int channels = 0;
    auto* data = stbi_load_from_memory(bytes.items, (int)bytes.count, &width, &height, &channels, 4);
    if (!data) {
        return Error::from_string_literal("could not load image");
    }
    Defer free_image = [&] {
        stbi_image_free(data);
    };
    usize size = usize(width) * usize(height) * usize(channels);
    auto image = TRY(Vector<f32>::create(size));
    for (usize i = 0; i < size; i++) {
        TRY(image.append(f32(data[i]) / 255.0f));
    }

    return Image {
        move(image),
        usize(width),
        usize(height),
        usize(channels)
    };
}
