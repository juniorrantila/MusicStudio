#include "./StringBuffer.h"

namespace Ty {

StringView StringBuffer::uppercase()
{
    char* data = mutable_data();
    for (usize i = 0; i < size(); i++) {
        char c = data[i];
        data[i] = c + (c >= 'a' & c <= 'z') * ('A' - 'a');
    }
    return view();
}

}
