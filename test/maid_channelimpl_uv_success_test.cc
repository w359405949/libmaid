#include <uv.h>
#include "gtest/gtest.h"

#include "test.pb.h"
#include "channelimpl.h"
#include "maid/controller.h"
#include "maid/controller.pb.h"
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
    return 0;
}

UV_EXTERN int uv_run(uv_loop_t*, uv_run_mode mode)
{
    return 0;
}

UV_EXTERN int uv_tcp_connect(uv_connect_t* req,
                             uv_tcp_t* handle,
                             const struct sockaddr* addr,
                             uv_connect_cb cb)
{
    //cb(req, 0);
    return 0;
}

UV_EXTERN int uv_tcp_init(uv_loop_t*, uv_tcp_t* handle)
{
    return 0;
}

UV_EXTERN void uv_close(uv_handle_t* handle, uv_close_cb close_cb)
{
}

UV_EXTERN int uv_listen(uv_stream_t* stream, int backlog, uv_connection_cb cb)
{
    return 0;
}

UV_EXTERN int uv_read_start(uv_stream_t*,
                            uv_alloc_cb alloc_cb,
                            uv_read_cb read_cb)
{

    return 0;
}

UV_EXTERN int uv_read_stop(uv_stream_t*)
{
    return 0;
}

UV_EXTERN int uv_accept(uv_stream_t* server, uv_stream_t* client)
{
    return 0;
}

UV_EXTERN int uv_tcp_bind(uv_tcp_t* handle,
                          const struct sockaddr* addr,
                          unsigned int flags)
{
    return 0;
}

UV_EXTERN int uv_timer_start(uv_timer_t* handle,
                             uv_timer_cb cb,
                             uint64_t timeout,
                             uint64_t repeat)
{
    return 0;
}

UV_EXTERN int uv_timer_stop(uv_timer_t* handle)
{
    return 0;
}

TEST(ChannelImpl, Constructor)
{
    maid::ChannelImpl channelimpl;

    ASSERT_EQ(NULL, channelimpl.default_stream_);
    ASSERT_EQ(uv_default_loop(), channelimpl.loop_);
    ASSERT_EQ(0u, channelimpl.service_.size());
    ASSERT_EQ(0u, channelimpl.middleware_.size());
    ASSERT_EQ(0u, channelimpl.connected_handle_.size());
    ASSERT_EQ(0u, channelimpl.listen_handle_.size());
    ASSERT_EQ(0u, channelimpl.timer_handle_.size());
    ASSERT_EQ(0u, channelimpl.idle_handle_.size());
    ASSERT_EQ(0u, channelimpl.buffer_.size());
    ASSERT_EQ(0u, channelimpl.transactions_.size());
    ASSERT_EQ(0u, channelimpl.async_result_.size());
    ASSERT_EQ(0u, channelimpl.sending_buffer_.size());
    ASSERT_EQ(0u, channelimpl.remote_closure_pool_.size());

    uv_loop_t* loop = uv_loop_new();
    maid::ChannelImpl channelimpl1(loop);

    ASSERT_EQ(NULL, channelimpl.default_stream_);
    ASSERT_EQ(uv_default_loop(), channelimpl.loop_);
    ASSERT_EQ(0u, channelimpl.service_.size());
    ASSERT_EQ(0u, channelimpl.middleware_.size());
    ASSERT_EQ(0u, channelimpl.connected_handle_.size());
    ASSERT_EQ(0u, channelimpl.listen_handle_.size());
    ASSERT_EQ(0u, channelimpl.timer_handle_.size());
    ASSERT_EQ(0u, channelimpl.idle_handle_.size());
    ASSERT_EQ(0u, channelimpl.buffer_.size());
    ASSERT_EQ(0u, channelimpl.transactions_.size());
    ASSERT_EQ(0u, channelimpl.async_result_.size());
    ASSERT_EQ(0u, channelimpl.sending_buffer_.size());
    ASSERT_EQ(0u, channelimpl.remote_closure_pool_.size());
}

TEST(ChannelImpl, CallMethod)
{
    maid::ChannelImpl channelimpl;

    //channelimpl.CallMethod();
}

TEST(ChannelImpl, Listen)
{
    maid::ChannelImpl channelimpl;
    int64_t listen_id = channelimpl.Listen("0.0.0.0", 8888);

    ASSERT_EQ(1u, channelimpl.listen_handle_.size());
    ASSERT_TRUE(channelimpl.listen_handle_.end() != channelimpl.listen_handle_.find(listen_id));
    ASSERT_LT(0, listen_id);
}

TEST(ChannelImpl, ListenNULL)
{
    maid::ChannelImpl channelimpl;
    int64_t listen_id = channelimpl.Listen(NULL, 8888);

    ASSERT_EQ(0u, channelimpl.listen_handle_.size());
    ASSERT_TRUE(channelimpl.listen_handle_.end() == channelimpl.listen_handle_.find(listen_id));
    ASSERT_GT(0, listen_id);
}

TEST(ChannelImpl, ListenInvalidHost)
{
    maid::ChannelImpl channelimpl;
    int64_t listen_id = channelimpl.Listen("abcdef", 8888);

    ASSERT_EQ(0u, channelimpl.listen_handle_.size());
    ASSERT_TRUE(channelimpl.listen_handle_.end() == channelimpl.listen_handle_.find(listen_id));
    ASSERT_GT(0, listen_id);
}

/*
 * uv_ip4_addr return success while port invalid(large than  65535)
 */
TEST(ChannelImpl, ListenInvalidPort)
{
    maid::ChannelImpl channelimpl;
    int64_t listen_id = channelimpl.Listen("0.0.0.0", 88888);

    ASSERT_EQ(1u, channelimpl.listen_handle_.size());
    ASSERT_TRUE(channelimpl.listen_handle_.end() != channelimpl.listen_handle_.find(listen_id));
    ASSERT_LT(0, listen_id);
}

TEST(ChannelImpl, Connect)
{
    maid::ChannelImpl channelimpl;
    int64_t connection_id = channelimpl.Connect("0.0.0.0", 8888);

    ASSERT_EQ(0u, channelimpl.connected_handle_.size());
    ASSERT_EQ(NULL, channelimpl.default_stream_);
    ASSERT_NE(0, connection_id);
}

TEST(ChannelImpl, ConnectAsDefault)
{
    maid::ChannelImpl channelimpl;
    int64_t connection_id = channelimpl.Connect("0.0.0.0", 8888, true);

    ASSERT_EQ(0u, channelimpl.connected_handle_.size());
    ASSERT_EQ(connection_id, (int64_t)channelimpl.default_stream_);
    ASSERT_NE(0, connection_id);
}

TEST(ChannelImpl, AppendService)
{
    maid::ChannelImpl channelimpl;
    TestServiceImpl* service = new TestServiceImpl();
    channelimpl.AppendService(service);

    ASSERT_EQ(1u, channelimpl.service_.size());
    ASSERT_TRUE(channelimpl.service_.end() != channelimpl.service_.find(service->GetDescriptor()->full_name()));
}

TEST(ChannelImpl, SendRequest)
{
    maid::ChannelImpl channelimpl;
    int64_t connection_id = channelimpl.Connect("0.0.0.0", 8888, true);

    maid::Controller controller;
    Request request;
    Response response;
    ClosureMock done;

    //channelimpl.SendRequest(&controller, &request, &response, &done);
    ASSERT_FALSE(controller.Failed());
    ASSERT_TRUE(controller.proto().stub());
    ASSERT_GE(1u, controller.proto().transmit_id());
    ASSERT_EQ(1u, channelimpl.transactions_.size());
    ASSERT_EQ(1u, channelimpl.transactions_[connection_id].size());
    ASSERT_EQ(1u, channelimpl.async_result_.size());

}

TEST(ChannelImpl, SendRequestNotConnected)
{
    maid::ChannelImpl channelimpl;

    Request request;
    Response response;
    ClosureMock done;
    maid::Controller controller;

    //channelimpl.SendRequest(&controller, &request, &response, &done);
    ASSERT_TRUE(controller.Failed());
    ASSERT_STREQ("not connected", controller.ErrorText().c_str());
    ASSERT_TRUE(controller.proto().stub());
}

TEST(ChannelImpl, SendResponse)
{
    maid::ChannelImpl channelimpl;
    maid::Controller controller;
    Response response;

    channelimpl.SendResponse(controller.mutable_proto(), &response);
    ASSERT_FALSE(controller.proto().stub());
}

TEST(ChannelImpl, DefaultConnectionGetSet)
{
    maid::ChannelImpl channelimpl;
    ASSERT_EQ(0, channelimpl.default_connection_id());

    int64_t connection_id = 1;
    channelimpl.set_default_connection_id(connection_id);
    ASSERT_EQ(0, channelimpl.default_connection_id());

    connection_id = channelimpl.Connect("0.0.0.0", 8888);
    channelimpl.set_default_connection_id(connection_id);
    ASSERT_EQ(0, channelimpl.default_connection_id());

    connection_id = channelimpl.Connect("0.0.0.0", 8888, true);
    channelimpl.set_default_connection_id(connection_id);
    ASSERT_EQ(connection_id, channelimpl.default_connection_id());
}

TEST(ChannelImpl, Update)
{
    maid::ChannelImpl channelimpl;
    channelimpl.Update();

    /*
     * NOTHING TO BE DONE
     */
}

TEST(ChannelImpl, HandleZero)
{
    maid::ChannelImpl channelimpl;
    channelimpl.AppendService(new TestServiceImpl());
    channelimpl.Connect("0.0.0.0", 8888, true);

    uv_buf_t* buf = (uv_buf_t*)malloc(sizeof(uv_buf_t));
    maid::ChannelImpl::OnAlloc((uv_handle_t*)channelimpl.default_stream_, 4, buf);
    ASSERT_GE(buf->len, 4);
    memset(buf->base, 0, buf->len);

    maid::ChannelImpl::OnRead(channelimpl.default_stream_, 4, buf);
    int32_t result = channelimpl.Handle((int64_t)channelimpl.default_stream_);

    ASSERT_EQ(0, result);
    ASSERT_EQ(channelimpl.buffer_[(int64_t)channelimpl.default_stream_].start, channelimpl.buffer_[(int64_t)channelimpl.default_stream_].end);
}

TEST(ChannelImpl, HandleLackData1)
{
    maid::ChannelImpl channelimpl;
    channelimpl.AppendService(new TestServiceImpl());
    channelimpl.Connect("0.0.0.0", 8888, true);

    uv_buf_t* buf = (uv_buf_t*)malloc(sizeof(uv_buf_t));
    maid::ChannelImpl::OnAlloc((uv_handle_t*)channelimpl.default_stream_, 2, buf);
    ASSERT_GE(buf->len, 4);

    maid::ChannelImpl::OnRead(channelimpl.default_stream_, 2, buf);
    int32_t result = channelimpl.Handle((int64_t)channelimpl.default_stream_);

    ASSERT_EQ(ERROR_LACK_DATA, result);
    ASSERT_EQ(channelimpl.buffer_[(int64_t)channelimpl.default_stream_].start + 2, channelimpl.buffer_[(int64_t)channelimpl.default_stream_].end);
}

TEST(ChannelImpl, HandleLackData2)
{
    maid::ChannelImpl channelimpl;
    channelimpl.AppendService(new TestServiceImpl());
    channelimpl.Connect("0.0.0.0", 8888, true);

    uv_buf_t* buf = (uv_buf_t*)malloc(sizeof(uv_buf_t));
    maid::ChannelImpl::OnAlloc((uv_handle_t*)channelimpl.default_stream_, 6, buf);
    ASSERT_GE(buf->len, 4);

    int32_t nlen = ::htonl(10);
    memcpy(buf->base, &nlen, sizeof(nlen));
    maid::ChannelImpl::OnRead(channelimpl.default_stream_, 6, buf);

    int32_t result = channelimpl.Handle((int64_t)channelimpl.default_stream_);
    ASSERT_EQ(ERROR_LACK_DATA, result);
    ASSERT_EQ(channelimpl.buffer_[(int64_t)channelimpl.default_stream_].start + 6, channelimpl.buffer_[(int64_t)channelimpl.default_stream_].end);
}

TEST(ChannelImpl, HandleOutOfSize)
{
    maid::ChannelImpl channelimpl;
    channelimpl.AppendService(new TestServiceImpl());
    channelimpl.Connect("0.0.0.0", 8888, true);

    uv_buf_t* buf = (uv_buf_t*)malloc(sizeof(uv_buf_t));
    maid::ChannelImpl::OnAlloc((uv_handle_t*)channelimpl.default_stream_, 6, buf);
    ASSERT_GE(buf->len, 4);

    int32_t nlen = ::htonl(channelimpl.controller_max_size_ + 1);
    memcpy(buf->base, &nlen, sizeof(nlen));
    maid::ChannelImpl::OnRead(channelimpl.default_stream_, 6, buf);

    int32_t result = channelimpl.Handle((int64_t)channelimpl.default_stream_);
    ASSERT_EQ(ERROR_OUT_OF_SIZE, result);
    ASSERT_EQ(channelimpl.buffer_[(int64_t)channelimpl.default_stream_].start, channelimpl.buffer_[(int64_t)channelimpl.default_stream_].end);
}


TEST(ChannelImpl, HandleParseFailed)
{
    maid::ChannelImpl channelimpl;
    channelimpl.AppendService(new TestServiceImpl());
    channelimpl.Connect("0.0.0.0", 8888, true);

    uv_buf_t* buf = (uv_buf_t*)malloc(sizeof(uv_buf_t));
    maid::ChannelImpl::OnAlloc((uv_handle_t*)channelimpl.default_stream_, 6, buf);
    ASSERT_GE(buf->len, 4);

    int32_t nlen = ::htonl(2);
    memcpy(buf->base, &nlen, sizeof(nlen));
    memcpy(buf->base + sizeof(nlen), "a", 1);
    maid::ChannelImpl::OnRead(channelimpl.default_stream_, 6, buf);

    int32_t result = channelimpl.Handle((int64_t)channelimpl.default_stream_);
    ASSERT_EQ(ERROR_PARSE_FAILED, result);
    ASSERT_EQ(channelimpl.buffer_[(int64_t)channelimpl.default_stream_].start, channelimpl.buffer_[(int64_t)channelimpl.default_stream_].end);
}

TEST(ChannelImpl, HandleRequest)
{
    maid::ChannelImpl channelimpl;
    maid::proto::ControllerProto* proto = new maid::proto::ControllerProto();
    channelimpl.AppendService(new TestServiceImpl());
    proto->set_full_service_name("TestService");
    proto->set_method_name("TestMethod");

    Request request;
    request.SerializeToString(proto->mutable_message());

    int32_t result = channelimpl.HandleRequest(proto);

    ASSERT_EQ(0, result);
    ASSERT_TRUE(proto->failed());
    ASSERT_EQ("not implement", proto->error_text());
}

TEST(ChannelImpl, HandleRequestNoService)
{
    maid::ChannelImpl channelimpl;
    maid::proto::ControllerProto* proto = new maid::proto::ControllerProto();
    proto->set_full_service_name("TestService");
    proto->set_method_name("TestMethod");

    Request request;
    request.SerializeToString(proto->mutable_message());

    int32_t result = channelimpl.HandleRequest(proto);

    ASSERT_EQ(ERROR_OTHER, result);
    ASSERT_TRUE(proto->failed());
    ASSERT_EQ("service TestService not exist", proto->error_text());
}


TEST(ChannelImpl, HandleRequestNoMethod)
{
    maid::ChannelImpl channelimpl;
    maid::proto::ControllerProto* proto = new maid::proto::ControllerProto();
    channelimpl.AppendService(new TestServiceImpl());
    proto->set_full_service_name("TestService");
    proto->set_method_name("TestNoSuchMethod");

    Request request;
    request.SerializeToString(proto->mutable_message());

    int32_t result = channelimpl.HandleRequest(proto);

    ASSERT_EQ(ERROR_OTHER, result);
    ASSERT_TRUE(proto->failed());
    ASSERT_EQ("method TestNoSuchMethod not exist", proto->error_text());
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

    channelimpl.SendRequest(controller->mutable_proto());
    int32_t result = channelimpl.HandleResponse(controller->mutable_proto());

    ASSERT_EQ(0, result);
    ASSERT_EQ(0u, channelimpl.async_result_.size());
    ASSERT_EQ(0u, channelimpl.transactions_[(int64_t)handle].size());
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
    channelimpl.SendRequest(controller->mutable_proto());

    controller->SetFailed("failed");
    int32_t result = channelimpl.HandleResponse(controller->mutable_proto());

    ASSERT_EQ(0, result);
    ASSERT_EQ(0u, channelimpl.async_result_.size());
    ASSERT_EQ(0u, channelimpl.transactions_[(int64_t)handle].size());
    ASSERT_EQ(1u, channelimpl.transactions_.size());
}

TEST(ChannelImpl, HandleResponseNoRequest)
{
    maid::ChannelImpl channelimpl;
    uv_stream_t* handle = (uv_stream_t*)malloc(sizeof(uv_stream_t));
    channelimpl.AddConnection(handle);

    maid::Controller* controller = new maid::Controller();
    int32_t result = channelimpl.HandleResponse(controller->mutable_proto());

    ASSERT_EQ(0, result);
    ASSERT_EQ(0u, channelimpl.async_result_.size());
    ASSERT_EQ(0u, channelimpl.transactions_[(int64_t)handle].size());
    ASSERT_EQ(1u, channelimpl.transactions_.size());
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
    ASSERT_EQ(1u, channelimpl.connected_handle_.size());
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
    controller->set_connection_id((int64_t)handle);
    Request request;
    Response response;
    ClosureMock done;
    channelimpl.SendRequest(controller->mutable_proto());
    ASSERT_FALSE(controller->Failed());

    uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
    req->data = controller;
    req->handle = (uv_stream_t*)malloc(sizeof(uv_stream_t));
    req->handle->data = &channelimpl;
    maid::ChannelImpl::AfterSendRequest(req, 0);

    ASSERT_EQ(1u, channelimpl.connected_handle_.size());
    ASSERT_EQ(1u, channelimpl.async_result_.size());
    ASSERT_EQ(1u, channelimpl.transactions_.size());
    ASSERT_EQ(1u, channelimpl.transactions_[(int64_t)handle].size());
}

TEST(ChannelImpl, AfterSendRequestWithFailed)
{
    maid::ChannelImpl channelimpl;
    uv_stream_t* handle = (uv_stream_t*)malloc(sizeof(uv_stream_t));
    channelimpl.AddConnection(handle);
    ASSERT_EQ(1u, channelimpl.connected_handle_.size());

    maid::Controller* controller = new maid::Controller();
    controller->set_connection_id((int64_t)handle);
    Request request;
    Response response;
    ClosureMock done;
    channelimpl.SendRequest(controller->mutable_proto());
    ASSERT_FALSE(controller->Failed());

    uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
    req->data = controller;
    req->handle = (uv_stream_t*)malloc(sizeof(uv_stream_t));
    req->handle->data = &channelimpl;
    maid::ChannelImpl::AfterSendRequest(req, -1);

    ASSERT_EQ(1u, channelimpl.connected_handle_.size());
    ASSERT_EQ(0u, channelimpl.async_result_.size());
    ASSERT_EQ(1u, channelimpl.transactions_.size());
    ASSERT_EQ(0u, channelimpl.transactions_[(int64_t)handle].size());
}

TEST(ChannelImpl, AfterSendResponse)
{
    uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
    maid::ChannelImpl::AfterSendResponse(req, 0);

     // NOTHING TO BE DONE
}

//TEST(ChannelImpl, OnClose)
//{
//    uv_handle_t* handle = (uv_handle_t*)malloc(sizeof(uv_handle_t));
//    maid::ChannelImpl::OnClose(handle);
//
//     // NOTHING TO BE DONE
//}

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
