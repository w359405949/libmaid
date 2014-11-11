#include <uv.h>
#include "gtest/gtest.h"

#include "test.pb.h"
#include "channelimpl.h"
#include "maid/controller.h"
#include "controller.pb.h"
#include "define.h"

class TestServiceImpl : public TestService
{
public:
    void TestMethod(google::protobuf::RpcController* controller,
            const Request* request,
            Response* response,
            google::protobuf::Closure* done)
    {
    }
};

class ClosureMock : public google::protobuf::Closure
{
public:
    void Run()
    {
    }
};

UV_EXTERN int uv_write(uv_write_t* req,
                       uv_stream_t* handle,
                       const uv_buf_t bufs[],
                       unsigned int nbufs,
                       uv_write_cb cb)
{
    return -1;
}

UV_EXTERN int uv_run(uv_loop_t*, uv_run_mode mode)
{
    return -1;
}

UV_EXTERN int uv_tcp_connect(uv_connect_t* req,
                             uv_tcp_t* handle,
                             const struct sockaddr* addr,
                             uv_connect_cb cb)
{
    return -1;
}

UV_EXTERN void uv_close(uv_handle_t* handle, uv_close_cb close_cb)
{
}

UV_EXTERN int uv_listen(uv_stream_t* stream, int backlog, uv_connection_cb cb)
{
    return -1;
}

UV_EXTERN int uv_read_start(uv_stream_t*,
                            uv_alloc_cb alloc_cb,
                            uv_read_cb read_cb)
{
    return -1;
}

UV_EXTERN int uv_read_stop(uv_stream_t*)
{
    return -1;
}

UV_EXTERN int uv_accept(uv_stream_t* server, uv_stream_t* client)
{
    return -1;
}

UV_EXTERN int uv_tcp_bind(uv_tcp_t* handle,
                          const struct sockaddr* addr,
                          unsigned int flags)
{
    return -1;
}

TEST(ChannelImpl, Constructor)
{
    maid::ChannelImpl channelimpl;

    ASSERT_EQ(0u, channelimpl.default_fd_);
    ASSERT_TRUE(NULL != channelimpl.loop_);
    ASSERT_EQ(10000u, channelimpl.controller_max_length_);
    ASSERT_EQ(0u, channelimpl.service_.size());
    ASSERT_EQ(0u, channelimpl.async_result_.size());
    ASSERT_EQ(0u, channelimpl.connected_handle_.size());
    ASSERT_EQ(0u, channelimpl.listen_handle_.size());
    ASSERT_EQ(0u, channelimpl.buffer_.size());
    ASSERT_EQ(0u, channelimpl.transactions_.size());
    ASSERT_EQ(0u, channelimpl.remote_closure_pool_.size());


    uv_loop_t* loop = uv_loop_new();
    maid::ChannelImpl channelimpl1(loop);

    ASSERT_EQ(0u, channelimpl1.default_fd_);
    ASSERT_EQ(loop, channelimpl1.loop_);
    ASSERT_EQ(10000u, channelimpl1.controller_max_length_);
    ASSERT_EQ(0u, channelimpl1.service_.size());
    ASSERT_EQ(0u, channelimpl1.async_result_.size());
    ASSERT_EQ(0u, channelimpl1.connected_handle_.size());
    ASSERT_EQ(0u, channelimpl1.listen_handle_.size());
    ASSERT_EQ(0u, channelimpl1.buffer_.size());
    ASSERT_EQ(0u, channelimpl1.transactions_.size());
    ASSERT_EQ(0u, channelimpl1.remote_closure_pool_.size());
}

TEST(ChannelImpl, CallMethod)
{
    maid::ChannelImpl channelimpl;

    //channelimpl.CallMethod();
}

TEST(ChannelImpl, Listen)
{
    maid::ChannelImpl channelimpl;
    int64_t fd = channelimpl.Listen("0.0.0.0", 8888);

    ASSERT_EQ(0u, channelimpl.listen_handle_.size());
    ASSERT_TRUE(channelimpl.listen_handle_.end() == channelimpl.listen_handle_.find(fd));
    ASSERT_GT(0, fd);
}

TEST(ChannelImpl, ListenNULL)
{
    maid::ChannelImpl channelimpl;
    int64_t fd = channelimpl.Listen(NULL, 8888);

    ASSERT_EQ(0u, channelimpl.listen_handle_.size());
    ASSERT_TRUE(channelimpl.listen_handle_.end() == channelimpl.listen_handle_.find(fd));
    ASSERT_GT(0, fd);
}

TEST(ChannelImpl, ListenInvalidHost)
{
    maid::ChannelImpl channelimpl;
    int64_t fd = channelimpl.Listen("abcdef", 8888);

    ASSERT_EQ(0u, channelimpl.listen_handle_.size());
    ASSERT_TRUE(channelimpl.listen_handle_.end() == channelimpl.listen_handle_.find(fd));
    ASSERT_GT(0, fd);
}

/*
 *
 * uv_ip4_addr return success while port invalid(large than  65535)
 */
TEST(ChannelImpl, ListenInvalidPort)
{
    maid::ChannelImpl channelimpl;
    int64_t fd = channelimpl.Listen("0.0.0.0", 88888);

    ASSERT_EQ(0u, channelimpl.listen_handle_.size());
    ASSERT_TRUE(channelimpl.listen_handle_.end() == channelimpl.listen_handle_.find(fd));
    ASSERT_GT(0, fd);
}

TEST(ChannelImpl, Connect)
{
    maid::ChannelImpl channelimpl;
    int64_t fd = channelimpl.Connect("0.0.0.0", 8888);

    ASSERT_EQ(0u, channelimpl.connected_handle_.size());
    ASSERT_EQ(0, channelimpl.default_fd_);
    ASSERT_EQ(ERROR_OTHER, fd);

    int64_t fd1 = channelimpl.Connect("0.0.0.0", 8888, true);

    ASSERT_EQ(0u, channelimpl.connected_handle_.size());
    ASSERT_NE(fd1, channelimpl.default_fd_);
    ASSERT_EQ(ERROR_OTHER, fd1);
}

TEST(ChannelImpl, AppendService)
{
    maid::ChannelImpl channelimpl;
    TestServiceImpl* service = new TestServiceImpl();
    channelimpl.AppendService(service);

    ASSERT_EQ(1u, channelimpl.service_.size());
    ASSERT_TRUE(channelimpl.service_.end() != channelimpl.service_.find(service->GetDescriptor()->full_name()));
}

TEST(ChannelImpl, SendRequestNotConnected)
{
    maid::ChannelImpl channelimpl;

    Request request;
    Response response;
    ClosureMock done;
    maid::Controller controller;

    channelimpl.SendRequest(&controller, &request, &response, &done);
    ASSERT_TRUE(controller.Failed());
    ASSERT_STREQ("not connected", controller.ErrorText().c_str());
    ASSERT_TRUE(controller.meta_data().stub());
}

TEST(ChannelImpl, SendRequest)
{
    maid::ChannelImpl channelimpl;
    uv_stream_t* handle = (uv_stream_t*)malloc(sizeof(uv_stream_t));
    channelimpl.AddConnection(handle);

    maid::Controller controller;
    controller.set_fd(channelimpl.StreamToFd(handle));
    Request request;
    Response response;
    ClosureMock done;

    channelimpl.SendRequest(&controller, &request, &response, &done);
    ASSERT_TRUE(controller.Failed());
    ASSERT_TRUE(controller.meta_data().stub());
    ASSERT_LT(0u, controller.meta_data().transmit_id());
    ASSERT_EQ(1u, channelimpl.transactions_.size());
    ASSERT_EQ(0u, channelimpl.transactions_[channelimpl.StreamToFd(handle)].size());
    ASSERT_EQ(0u, channelimpl.async_result_.size());

}

TEST(ChannelImpl, SendResponse)
{
    maid::ChannelImpl channelimpl;
    maid::Controller controller;
    Response response;

    channelimpl.SendResponse(&controller, &response);
    ASSERT_FALSE(controller.meta_data().stub());
}

TEST(ChannelImpl, SendNotify)
{
    maid::ChannelImpl channelimpl;
    maid::Controller controller;
    Request request;

    channelimpl.SendNotify(&controller, &request);
    ASSERT_TRUE(controller.meta_data().stub());
}

TEST(ChannelImpl, DefaultFdGetSet)
{
    maid::ChannelImpl channelimpl;
    ASSERT_EQ(0, channelimpl.default_fd());

    channelimpl.set_default_fd(1);
    ASSERT_EQ(1, channelimpl.default_fd());
}

TEST(ChannelImpl, Update)
{
    maid::ChannelImpl channelimpl;
    channelimpl.Update();

    /*
     * NOTHING TO BE DONE
     */
}

TEST(ChannelImpl, Handle)
{
    maid::ChannelImpl channelimpl;
    channelimpl.AppendService(new TestServiceImpl());
    uv_stream_t* handle = (uv_stream_t*)malloc(sizeof(uv_stream_t));
    handle->data = &channelimpl;
    int64_t fd = channelimpl.StreamToFd(handle);

    uv_buf_t* buf = (uv_buf_t*)malloc(sizeof(uv_buf_t));

    maid::Controller controller;
    controller.meta_data().set_full_service_name("TestService");
    controller.meta_data().set_method_name("TestMethod");

    //request
    controller.meta_data().set_stub(true);
    controller.meta_data().set_notify(false);
    maid::ChannelImpl::OnAlloc((uv_handle_t*)handle, 1000, buf);
    ASSERT_TRUE(buf->base != NULL);

    uint32_t len = controller.meta_data().ByteSize();
    uint32_t nlen = ::htonl(len);
    memcpy(buf->base, &nlen, 4);
    bool sr = controller.meta_data().SerializeToArray((int8_t*)buf->base + sizeof(uint32_t), len);
    ASSERT_TRUE(sr);

    int32_t result = channelimpl.Handle(handle, sizeof(uint32_t) + len);
    ASSERT_EQ(0, result);
    ASSERT_EQ(0u, channelimpl.buffer_[fd].len);

    //notify
    controller.meta_data().set_stub(true);
    controller.meta_data().set_notify(true);
    maid::ChannelImpl::OnAlloc((uv_handle_t*)handle, 1000, buf);
    ASSERT_TRUE(buf->base != NULL);

    len = controller.meta_data().ByteSize();
    nlen = ::htonl(len);
    memcpy(buf->base, &nlen, 4);
    sr = controller.meta_data().SerializeToArray((int8_t*)buf->base + sizeof(uint32_t), len);
    ASSERT_TRUE(sr);

    result = channelimpl.Handle(handle, sizeof(uint32_t) + len);
    ASSERT_EQ(0, result);
    ASSERT_EQ(0u, channelimpl.buffer_[fd].len);

    //response
    controller.meta_data().set_stub(false);
    controller.meta_data().set_notify(false);
    maid::ChannelImpl::OnAlloc((uv_handle_t*)handle, 1000, buf);
    ASSERT_TRUE(buf->base != NULL);

    len = controller.meta_data().ByteSize();
    nlen = ::htonl(len);
    memcpy(buf->base, &nlen, 4);
    sr = controller.meta_data().SerializeToArray((int8_t*)buf->base + sizeof(uint32_t), len);
    ASSERT_TRUE(sr);

    result = channelimpl.Handle(handle, sizeof(uint32_t) + len);
    ASSERT_EQ(0, result);
    ASSERT_EQ(0u, channelimpl.buffer_[fd].len);
}

TEST(ChannelImpl, HandleWithMore)
{
    maid::ChannelImpl channelimpl;
    channelimpl.AppendService(new TestServiceImpl());
    uv_stream_t* handle = (uv_stream_t*)malloc(sizeof(uv_stream_t));
    handle->data = &channelimpl;
    int64_t fd = channelimpl.StreamToFd(handle);
    uv_buf_t* buf = (uv_buf_t*)malloc(sizeof(uv_buf_t));
    maid::ChannelImpl::OnAlloc((uv_handle_t*)handle, 1000, buf);
    ASSERT_TRUE(buf->base != NULL);

    maid::Controller controller;
    controller.set_fd(fd);
    controller.meta_data().set_full_service_name("TestService");
    controller.meta_data().set_method_name("TestMethod");

    int32_t cur = 0;

    //request
    controller.meta_data().set_stub(true);
    controller.meta_data().set_notify(false);
    uint32_t len = controller.meta_data().ByteSize();
    uint32_t nlen = ::htonl(len);
    for (int32_t i = 0; i < 10; i++) {
        memcpy((int8_t*)buf->base + cur, &nlen, sizeof(uint32_t));
        bool sr = controller.meta_data().SerializeToArray((int8_t*)buf->base + sizeof(uint32_t) + cur, len);
        ASSERT_TRUE(sr);
        cur += sizeof(uint32_t) + len;
    }

    //notify
    controller.meta_data().set_stub(true);
    controller.meta_data().set_notify(true);
    len = controller.meta_data().ByteSize();
    nlen = ::htonl(len);
    for (int32_t i = 0; i < 10; i++) {
        memcpy((int8_t*)buf->base + cur, &nlen, sizeof(uint32_t));
        bool sr = controller.meta_data().SerializeToArray((int8_t*)buf->base + sizeof(uint32_t) + cur, len);
        ASSERT_TRUE(sr);
        cur += sizeof(uint32_t) + len;
    }

    //response
    controller.meta_data().set_stub(false);
    controller.meta_data().set_notify(false);
    len = controller.meta_data().ByteSize();
    nlen = ::htonl(len);
    for (int32_t i = 0; i < 10; i++) {
        memcpy((int8_t*)channelimpl.buffer_[fd].base + cur, &nlen, sizeof(uint32_t));
        bool sr = controller.meta_data().SerializeToArray((int8_t*)channelimpl.buffer_[fd].base + sizeof(uint32_t) + cur, len);
        ASSERT_TRUE(sr);
        cur += sizeof(uint32_t) + len;
    }

    int32_t result = channelimpl.Handle(handle, cur);

    ASSERT_EQ(0, result);
    ASSERT_EQ(0u, channelimpl.buffer_[fd].len);
}

TEST(ChannelImpl, HandleParseFailed)
{
    maid::ChannelImpl channelimpl;
    channelimpl.buffer_[1].Expend(1000);
    uint32_t len = ::htonl(100);
    memcpy(channelimpl.buffer_[1].base, &len, 4);
    memcpy((int8_t*)channelimpl.buffer_[1].base + 4, "asda", 4);
    int32_t result = channelimpl.Handle((uv_stream_t*)1, 1000);

    ASSERT_EQ(ERROR_PARSE_FAILED, result);
    ASSERT_EQ(896u, channelimpl.buffer_[1].len);
}

TEST(ChannelImpl, HandleLackData)
{
    maid::ChannelImpl channelimpl;
    channelimpl.buffer_[1].Expend(10);
    uint32_t len = ::htonl(100);
    memcpy(channelimpl.buffer_[1].base, &len, 4);
    int32_t result = channelimpl.Handle((uv_stream_t*)1, 10);

    ASSERT_EQ(ERROR_LACK_DATA, result);
    ASSERT_EQ(10u, channelimpl.buffer_[1].len);
}

TEST(ChannelImpl, HandleRequest)
{
    maid::ChannelImpl channelimpl;
    maid::Controller* controller = new maid::Controller();
    channelimpl.AppendService(new TestServiceImpl());
    controller->meta_data().set_full_service_name("TestService");
    controller->meta_data().set_method_name("TestMethod");

    Request request;
    request.SerializeToString(controller->meta_data().mutable_message());

    int32_t result = channelimpl.HandleRequest(controller);

    ASSERT_EQ(0, result);
    ASSERT_EQ(0u, channelimpl.buffer_[1].len);
}

TEST(ChannelImpl, HandleRequestNoService)
{
    maid::ChannelImpl channelimpl;
    maid::Controller* controller = new maid::Controller();
    controller->meta_data().set_full_service_name("TestService");
    controller->meta_data().set_method_name("TestMethod");

    Request request;
    request.SerializeToString(controller->meta_data().mutable_message());

    int32_t result = channelimpl.HandleRequest(controller);

    ASSERT_EQ(ERROR_OTHER, result);
    ASSERT_EQ(0u, channelimpl.buffer_[1].len);
}

TEST(ChannelImpl, HandleResponse)
{
    maid::ChannelImpl channelimpl;
    uv_stream_t* handle = (uv_stream_t*)malloc(sizeof(uv_stream_t));
    channelimpl.AddConnection(handle);
    maid::Controller* controller = new maid::Controller();
    Request request;
    Response response;
    ClosureMock done;

    channelimpl.SendRequest(controller, &request, &response, &done);
    int32_t result = channelimpl.HandleResponse(controller);

    ASSERT_EQ(0, result);
    ASSERT_EQ(0u, channelimpl.async_result_.size());
    ASSERT_EQ(0u, channelimpl.transactions_[channelimpl.StreamToFd(handle)].size());
    ASSERT_EQ(1u, channelimpl.transactions_.size());
}

TEST(ChannelImpl, HandleResponseWithFailed)
{
    maid::ChannelImpl channelimpl;
    uv_stream_t* handle = (uv_stream_t*)malloc(sizeof(uv_stream_t));
    channelimpl.AddConnection(handle);

    maid::Controller* controller = new maid::Controller();
    Request request;
    Response response;
    ClosureMock done;
    channelimpl.SendRequest(controller, &request, &response, &done);

    controller->SetFailed("failed");
    int32_t result = channelimpl.HandleResponse(controller);

    ASSERT_EQ(0, result);
    ASSERT_EQ(0u, channelimpl.async_result_.size());
    ASSERT_EQ(0u, channelimpl.transactions_[channelimpl.StreamToFd(handle)].size());
    ASSERT_EQ(1u, channelimpl.transactions_.size());
}

TEST(ChannelImpl, HandleResponseNoRequest)
{
    maid::ChannelImpl channelimpl;
    uv_stream_t* handle = (uv_stream_t*)malloc(sizeof(uv_stream_t));
    channelimpl.AddConnection(handle);

    maid::Controller* controller = new maid::Controller();
    int32_t result = channelimpl.HandleResponse(controller);

    ASSERT_EQ(0, result);
    ASSERT_EQ(0u, channelimpl.async_result_.size());
    ASSERT_EQ(0u, channelimpl.transactions_[channelimpl.StreamToFd(handle)].size());
    ASSERT_EQ(1u, channelimpl.transactions_.size());
}

TEST(ChannelImpl, HandleNotify)
{
    maid::ChannelImpl channelimpl;
    maid::Controller* controller = new maid::Controller();
    controller->meta_data().set_stub(true);
    controller->meta_data().set_notify(true);
    channelimpl.AppendService(new TestServiceImpl());
    controller->meta_data().set_full_service_name("TestService");
    controller->meta_data().set_method_name("TestMethod");

    Request request;
    request.SerializeToString(controller->meta_data().mutable_message());

    int32_t result = channelimpl.HandleRequest(controller);

    ASSERT_EQ(0, result);
    ASSERT_EQ(0u, channelimpl.buffer_[1].len);
}

TEST(ChannelImpl, RemoteClosureNewAndDelete)
{
    maid::ChannelImpl channel;
    ASSERT_EQ(0u, channel.remote_closure_pool_.size());

    maid::RemoteClosure* closure = channel.NewRemoteClosure();
    ASSERT_EQ(0u, channel.remote_closure_pool_.size());
    ASSERT_TRUE(NULL != closure);

    channel.DeleteRemoteClosure(closure);
    ASSERT_EQ(1u, channel.remote_closure_pool_.size());

    maid::RemoteClosure* closure1 = channel.NewRemoteClosure();
    ASSERT_EQ(0u, channel.remote_closure_pool_.size());
    ASSERT_EQ(closure1, closure);

}

TEST(ChannelImpl, OnRead)
{
    maid::ChannelImpl channelimpl;
    uv_stream_t* handle = (uv_stream_t*)malloc(sizeof(uv_stream_t));
    channelimpl.AddConnection(handle);
    uv_buf_t* buf = (uv_buf_t*)malloc(sizeof(uv_buf_t));
    maid::ChannelImpl::OnAlloc((uv_handle_t*)handle, 10, buf);
    ASSERT_EQ(1u, channelimpl.buffer_.size());
    maid::ChannelImpl::OnRead(handle, buf->len, buf);

    ASSERT_EQ(1u, channelimpl.connected_handle_.size());
}

TEST(ChannelImpl, OnReadWithFailed)
{
    return;
    maid::ChannelImpl channelimpl;
    uv_stream_t* handle = (uv_stream_t*)malloc(sizeof(uv_stream_t));
    channelimpl.AddConnection(handle);
    uv_buf_t* buf = (uv_buf_t*)malloc(sizeof(uv_buf_t));
    maid::ChannelImpl::OnAlloc((uv_handle_t*)handle, 10, buf);
    ASSERT_EQ(1u, channelimpl.buffer_.size());
    maid::ChannelImpl::OnRead(handle, -1, buf);

    ASSERT_EQ(0u, channelimpl.connected_handle_.size());
}

TEST(ChannelImpl, OnAccept)
{
    maid::ChannelImpl channelimpl;
    uv_stream_t* stream = (uv_stream_t*)malloc(sizeof(uv_stream_t));
    stream->loop = channelimpl.loop_;
    stream->data = &channelimpl;

    maid::ChannelImpl::OnAccept(stream, 0);
    ASSERT_EQ(0u, channelimpl.connected_handle_.size());
}

TEST(ChannelImpl, OnAcceptWithFailed)
{
    maid::ChannelImpl channelimpl;
    uv_stream_t* stream = (uv_stream_t*)malloc(sizeof(uv_stream_t));
    stream->data = &channelimpl;

    maid::ChannelImpl::OnAccept(stream, -1);
    ASSERT_EQ(0u, channelimpl.connected_handle_.size());
}

TEST(ChannelImpl, OnConnect)
{
    maid::ChannelImpl channelimpl;
    uv_connect_t* req = (uv_connect_t*)malloc(sizeof(uv_connect_t));
    req->data = &channelimpl;

    uv_stream_t* handle = (uv_stream_t*)malloc(sizeof(uv_stream_t));
    handle->data = NULL;

    req->handle = handle;
    maid::ChannelImpl::OnConnect(req, 0);

    ASSERT_EQ(1u, channelimpl.connected_handle_.size());
    ASSERT_EQ(&channelimpl, handle->data);
}

TEST(ChannelImpl, OnConnectWithFailed)
{
    maid::ChannelImpl channelimpl;
    uv_connect_t* req = (uv_connect_t*)malloc(sizeof(uv_connect_t));
    req->data = &channelimpl;

    uv_stream_t* handle = (uv_stream_t*)malloc(sizeof(uv_stream_t));
    handle->data = NULL;
    req->handle = handle;
    maid::ChannelImpl::OnConnect(req, -1);

    ASSERT_EQ(0u, channelimpl.connected_handle_.size());
    ASSERT_EQ(0u, channelimpl.transactions_.size());
    ASSERT_EQ(0u, channelimpl.async_result_.size());
    ASSERT_EQ(0u, channelimpl.buffer_.size());
    ASSERT_TRUE(NULL == handle->data);
}


TEST(ChannelImpl, OnAlloc)
{
    maid::ChannelImpl channelimpl;
    uv_handle_t* handle = (uv_handle_t*)malloc(sizeof(uv_handle_t));
    handle->data = &channelimpl;
    uv_buf_t* buf = (uv_buf_t*)malloc(sizeof(uv_buf_t));
    buf->base = NULL;
    maid::ChannelImpl::OnAlloc(handle, 10, buf);

    ASSERT_EQ(1u, channelimpl.buffer_.size());
    ASSERT_LE(10u, buf->len);
    ASSERT_TRUE(NULL != buf->base);
}

TEST(ChannelImpl, OnAllocWithZero)
{
    maid::ChannelImpl channelimpl;
    uv_handle_t* handle = (uv_handle_t*)malloc(sizeof(uv_handle_t));
    handle->data = &channelimpl;
    uv_buf_t* buf = (uv_buf_t*)malloc(sizeof(uv_buf_t));
    buf->base = NULL;
    maid::ChannelImpl::OnAlloc(handle, 0, buf);

    ASSERT_EQ(1u, channelimpl.buffer_.size());
    ASSERT_EQ(0u, buf->len);
    ASSERT_TRUE(NULL == buf->base);
}

TEST(ChannelImpl, AfterSendRequest)
{
    maid::ChannelImpl channelimpl;
    uv_stream_t* handle = (uv_stream_t*)malloc(sizeof(uv_stream_t));
    channelimpl.AddConnection(handle);
    maid::Controller* controller = new maid::Controller();
    controller->set_fd(channelimpl.StreamToFd(handle));
    Request request;
    Response response;
    ClosureMock done;
    channelimpl.SendRequest(controller, &request, &response, &done);
    ASSERT_TRUE(controller->Failed());

    uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
    req->data = controller;
    req->handle = (uv_stream_t*)malloc(sizeof(uv_stream_t));
    req->handle->data = &channelimpl;
    maid::ChannelImpl::AfterSendRequest(req, 0);

    ASSERT_EQ(1u, channelimpl.connected_handle_.size());
    ASSERT_EQ(0u, channelimpl.async_result_.size());
    ASSERT_EQ(1u, channelimpl.transactions_.size());
    ASSERT_EQ(0u, channelimpl.transactions_[channelimpl.StreamToFd(handle)].size());
}

TEST(ChannelImpl, AfterSendRequestWithFailed)
{
    maid::ChannelImpl channelimpl;
    uv_stream_t* handle = (uv_stream_t*)malloc(sizeof(uv_stream_t));
    channelimpl.AddConnection(handle);
    ASSERT_EQ(1u, channelimpl.connected_handle_.size());

    maid::Controller* controller = new maid::Controller();
    controller->set_fd(channelimpl.StreamToFd(handle));
    Request request;
    Response response;
    ClosureMock done;
    channelimpl.SendRequest(controller, &request, &response, &done);
    ASSERT_TRUE(controller->Failed());

    uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
    req->data = controller;
    req->handle = (uv_stream_t*)malloc(sizeof(uv_stream_t));
    req->handle->data = &channelimpl;
    maid::ChannelImpl::AfterSendRequest(req, -1);

    ASSERT_EQ(1u, channelimpl.connected_handle_.size());
    ASSERT_EQ(0u, channelimpl.async_result_.size());
    ASSERT_EQ(1u, channelimpl.transactions_.size());
    ASSERT_EQ(0u, channelimpl.transactions_[channelimpl.StreamToFd(handle)].size());
}

TEST(ChannelImpl, AfterSendNotify)
{
    uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
    maid::ChannelImpl::AfterSendNotify(req, 0);

    /*
     * NOTHING TO BE DONE
     */
}

TEST(ChannelImpl, AfterSendResponse)
{
    uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
    maid::ChannelImpl::AfterSendResponse(req, 0);

    /*
     * NOTHING TO BE DONE
     */
}

TEST(ChannelImpl, OnClose)
{
    uv_handle_t* handle = (uv_handle_t*)malloc(sizeof(uv_handle_t));
    maid::ChannelImpl::OnClose(handle);

    /*
     * NOTHING TO BE DONE
     */
}

TEST(ChannelImpl, AddConnection)
{
    maid::ChannelImpl channelimpl;
    uv_stream_t* handle = (uv_stream_t*)malloc(sizeof(uv_stream_t));

    channelimpl.AddConnection(handle);
    ASSERT_EQ(&channelimpl, handle->data);
    ASSERT_EQ(1u, channelimpl.connected_handle_.size());
}

TEST(ChannelImpl, RemoveConnection)
{
    maid::ChannelImpl channelimpl;
    uv_stream_t* handle = (uv_stream_t*)malloc(sizeof(uv_stream_t));

    channelimpl.AddConnection(handle);
    channelimpl.RemoveConnection(handle);

    ASSERT_EQ(0u, channelimpl.connected_handle_.size());
    ASSERT_EQ(0u, channelimpl.transactions_.size());
    ASSERT_EQ(0u, channelimpl.buffer_.size());
    ASSERT_EQ(0u, channelimpl.async_result_.size());
}

TEST(ChannelImpl, OnRemoteClosureGC)
{
}
