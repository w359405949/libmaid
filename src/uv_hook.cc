#include <stdlib.h>
#include "uv.h"

static uv_once_t loop_once;
static uv_key_t loop_key;

static void init_key()
{
    uv_key_create(&loop_key);
}

uv_loop_t* maid_default_loop()
{
    uv_once(&loop_once, init_key);

    uv_loop_t* loop = (uv_loop_t*)uv_key_get(&loop_key);
    if (loop == NULL) {
        loop = (uv_loop_t*)malloc(sizeof(uv_loop_t));
        uv_loop_init(loop);
        uv_key_set(&loop_key, loop);
    }

    return loop;

}

void maid_loop()
{
    uv_once(&loop_once, init_key);

    uv_loop_t* loop = (uv_loop_t*)uv_key_get(&loop_key);
    if (loop == NULL) {
        loop = (uv_loop_t*)malloc(sizeof(uv_loop_t));
        uv_key_set(&loop_key, NULL);
        free(loop);
    }
}
