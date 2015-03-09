#pragma once

#include <stddef.h>
#include <stdlib.h>

namespace maid
{

struct Buffer
{
    int8_t* start;
    int8_t* end;

    Buffer()
        :start(NULL),
        end(NULL),
        base_(NULL),
        size_(0)
    {
    }

    ~Buffer()
    {
        free(base_);
        start = NULL;
        end = NULL;
        base_ = NULL;
    }

    size_t Expend(size_t expect_size);

    void reset();

public:
    int8_t* base_;
    size_t size_;
};

}
