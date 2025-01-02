#pragma once

namespace Ty {

template <typename T>
void swap(T* a, T* b)
{
    T c = *a;
    T d = *b;
    *a = d;
    *b = c;
}

}

using Ty::swap;
