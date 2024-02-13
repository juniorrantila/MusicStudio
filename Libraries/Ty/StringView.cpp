#include "StringView.h"
#include "ErrorOr.h"
#include "Vector.h"
#include "StringBuffer.h"

namespace Ty {

ErrorOr<Vector<u32>> StringView::find_all(char character) const
{
    auto occurrences = TRY(Vector<u32>::create());

    for (u32 i = 0; i < size; i++) {
        if (data[i] == character)
            TRY(occurrences.append(i));
    }

    return occurrences;
}

ErrorOr<Vector<u32>> StringView::find_all(StringView sequence) const
{
    auto occurrences = TRY(Vector<u32>::create());

    for (u32 i = 0; i < size;) {
        if (sub_view(i, sequence.size) == sequence) {
            TRY(occurrences.append(i));
            i += sequence.size;
            continue;
        }
        i++;
    }

    return occurrences;
}

ErrorOr<Vector<StringView>> StringView::split_on(
    char character) const
{
    auto indexes = TRY(find_all(character));
    if (indexes.is_empty()) {
        auto v = TRY(Vector<StringView>::create(1));
        TRY(v.append(*this));
        return v;
    }

    auto splits = TRY(Vector<StringView>::create());

    u32 last_index = 0xFFFFFFFF; // Intentional overflow
    for (auto index : indexes) {
        TRY(splits.append(part(last_index + 1, index)));
        last_index = index;
    }
    TRY(splits.append(part(last_index + 1, size)));

    return splits;
}

ErrorOr<Vector<StringView>> StringView::split_on(
    StringView sequence) const
{
    auto indexes = TRY(find_all(sequence));
    if (indexes.is_empty()) {
        auto v = TRY(Vector<StringView>::create(1));
        TRY(v.append(*this));
        return v;
    }

    auto splits = TRY(Vector<StringView>::create());

    u32 last_index = 0xFFFFFFFF; // Intentional overflow
    for (auto index : indexes) {
        TRY(splits.append(part(last_index + sequence.size, index)));
        last_index = index;
    }
    TRY(splits.append(part(last_index + sequence.size, size)));

    return splits;
}

ErrorOr<StringBuffer> StringView::join(View<StringView const> parts) const
{
    auto buffer = TRY(StringBuffer::create());
    if (parts.size() == 0) {
        return buffer;
    }
    if (parts.size() == 1) {
        TRY(buffer.write(parts[0]));
        return buffer;
    }
    for (u32 i = 0; i < parts.size() - 1; i++) {
        TRY(buffer.write(parts[i], *this));
    }
    TRY(buffer.write(parts.last()));
    return buffer;
}

}
