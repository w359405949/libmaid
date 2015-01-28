
#include <glog/logging.h>
#include "buffer.h"
#include "define.h"
#include "string.h"

using maid::Buffer;

size_t Buffer::Expend(size_t expect)
{
    CHECK(start >= base_);
    CHECK(end >= start);

    size_t space_used = end - start;
    size_t remain = size_ - space_used;
    size_t tail = size_ - (end - base_);

    if (tail >= expect) {
        return tail;
    }

    if (remain >= expect) {
        memmove(base_, start, space_used);
        start = base_;
        end = base_ + space_used;
        return remain;
    }

    size_t new_size = size_;
    while (expect + space_used > new_size){
        new_size += 1;
        new_size <<= 1;
    }

    int8_t* new_ptr = (int8_t*)malloc(new_size);
    if (NULL == new_ptr){
        return tail;
    }

    memmove(new_ptr, start, space_used);

    free(base_);
    base_ = new_ptr;
    start = new_ptr;
    end = new_ptr + space_used;
    size_ = new_size;

    return new_size - space_used;
}
