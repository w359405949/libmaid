#ifndef _MAID_BUFFER_H_
#define _MAID_BUFFER_H_

#include <stddef.h>
#include <stdlib.h>

namespace maid
{

struct Buffer
{
    void* base;
    size_t len;
    size_t total;

    Buffer()
        :base(NULL),
        len(0),
        total(0)
    {
    }

    ~Buffer()
    {
        free(base);
    }

    void Expend(size_t expect_length);
};

}

#endif /*_MAID_BUFFER_H_*/
