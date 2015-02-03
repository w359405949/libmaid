#include "gtest/gtest.h"
#include "test.pb.h"
#include "channel.h"
#include "controller.h"
#include "closure.h"
#include <uv.h>

UV_EXTERN int uv_idle_init(uv_loop_t*, uv_idle_t* idle)
{
    return 0;
}

UV_EXTERN int uv_idle_start(uv_idle_t* idle, uv_idle_cb cb)
{
    return 0;
}

UV_EXTERN int uv_read_start(uv_stream_t*,
                            uv_alloc_cb alloc_cb,
                            uv_read_cb read_cb)
{
    return 0;
}


UV_EXTERN int uv_write(uv_write_t* req,
                       uv_stream_t* handle,
                       const uv_buf_t bufs[],
                       unsigned int nbufs,
                       uv_write_cb cb)
{
    return 0;
}

TEST(Clouse, Run)
{
}

TEST(TcpClosure, Run)
{
    uv_stream_t* stream = new uv_stream_t();
    maid::TcpChannel* channel = new maid::TcpChannel(stream, NULL);
    maid::Controller* controller = new maid::Controller();
    Request* request = new Request();
    Response* response = new Response();
    maid::TcpClosure* closure = new maid::TcpClosure(channel, controller, request, response);
    closure->Run();

    ASSERT_TRUE(closure->send_buffer() != NULL);
    ASSERT_EQ(closure->channel(), channel);
    ASSERT_EQ(closure->request(), request);
    ASSERT_EQ(closure->response(), response);
    ASSERT_EQ(closure->controller(), controller);
}
