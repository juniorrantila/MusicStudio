#include "./Pipeline.h"

#include <Ty/ErrorOr.h>
#include <Ty/Swap.h>

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
    return in;
}

}
