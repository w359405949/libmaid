#include "buffer.h"
#include "define.h"
#include "string.h"

using maid::Buffer;

void Buffer::Expend(size_t expect_length)
{
    size_t new_length = total;
    while (expect_length + len > new_length){
        new_length += 1;
        new_length <<= 1;
    }
    if (new_length == 0)
    {
        return;
    }

    void* new_ptr = malloc(new_length);
    if (NULL == new_ptr){
        return;
    }
    if (len != 0)
    {
        memmove(new_ptr, base, len);
    }
    free(base);
    base = new_ptr;
    total = new_length;
}
