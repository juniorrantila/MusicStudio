#include "./Pipeline.h"

#include <LibTy/ErrorOr.h>
#include <LibTy/Swap.h>

#include <string.h>

namespace AU {

ErrorOr<void> Pipeline::pipe(SmallCapture<void(f64*, f64*, usize, usize)> pipe)
{
    TRY(m_pipeline.append(move(pipe)));
    return {};
}

f64* Pipeline::run(f64* out, f64* in, usize frames, usize channels)
{
    for (auto& pipe : m_pipeline) {
        pipe(out, in, frames, channels);
        swap(&out, &in);
    }
    if (m_pipeline.size() % 2 == 0) {
        memcpy(out, in, sizeof(f64) * frames * channels);
    }
    return out;
}

}
