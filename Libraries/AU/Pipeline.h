#pragma once
#include "./Forward.h"

#include <Ty/Vector.h>
#include <Ty/SmallCapture.h>

namespace AU {

struct Pipeline {
    ErrorOr<void> pipe(SmallCapture<void(f64* out, f64* in, usize frames, usize channels)>);
    f64* run(f64* out, f64* in, usize frames, usize channels);

private:
    Vector<SmallCapture<void(f64*, f64*, usize, usize)>> m_pipeline {};
};

}
