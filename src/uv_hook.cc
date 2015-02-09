#include <stdlib.h>
#include "glog/logging.h"
#include "uv.h"

uv_once_t once_t;
uv_key_t loop_key;

void init_once()
{
    uv_key_create(&loop_key);
}

UV_EXTERN uv_loop_t* uv_default_loop(void)
{
    uv_once(&once_t, init_once);

    uv_loop_t* loop = (uv_loop_t*)uv_key_get(&loop_key);
    if (NULL == loop) {
        loop = (uv_loop_t*)malloc(sizeof(uv_loop_t));
        uv_loop_init(loop);
        uv_key_set(&loop_key, loop);
    }
    DLOG(INFO) << "loop:" << (int64_t)loop;
    return loop;
}
