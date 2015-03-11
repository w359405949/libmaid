#include "gtest/gtest.h"
#include "maid/controller.pb.h"
#include "wire_format.h"
#include "channel.h"
#include "controller.h"
#include "channel_factory.h"
#include "uv_hook.h"
#include "test.pb.h"

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

UV_EXTERN int uv_timer_start(uv_timer_t* handle,
                             uv_timer_cb cb,
                             uint64_t timeout,
                             uint64_t repeat)
{
    return 0;
}
UV_EXTERN int uv_timer_again(uv_timer_t* handle)
{
    return 0;
}

UV_EXTERN int uv_timer_stop(uv_timer_t* handle)
{
    return 0;
}

UV_EXTERN int uv_idle_start(uv_idle_t* idle, uv_idle_cb cb)
{
    return 0;
}

UV_EXTERN int uv_idle_stop(uv_idle_t* idle)
{
    return 0;
}

UV_EXTERN int uv_write(uv_write_t* req,
                       uv_stream_t* handle,
                       const uv_buf_t bufs[],
                       unsigned int nbufs,
                       uv_write_cb cb)
{
    req->handle = handle;
    return 0;
}

UV_EXTERN void uv_stop(uv_loop_t*)
{
}

class Closure : public google::protobuf::Closure
{
public:
    void Run()
    {
        request_ = NULL;
        response_ = NULL;
        controller_ = NULL;
    }

    Closure(maid::Controller* controller, Request* request, Response* response)
        :controller_(controller),
        request_(request),
        response_(response)
    {
    }

public:
    maid::Controller* controller_;
    Request* request_;
    Response* response_;
};

class TestServiceImpl : public TestService
{
public:
    void TestMethod(google::protobuf::RpcController* controller,
            const Request* request,
            Response* response,
            google::protobuf::Closure* done)
    {
        response->set_message(request->message());
    }
};


TEST(Channel, default_instance)
{
    ASSERT_TRUE(maid::Channel::default_instance() != NULL);
}

TEST(Channel, CallMethod)
{
    const google::protobuf::MethodDescriptor* method = google::protobuf::DescriptorPool::generated_pool()->FindMethodByName("TestMethod");
    Request* request = new Request();
    Response* response = new Response();
    maid::Controller* controller = new maid::Controller();
    Closure* closure = new Closure(controller, request, response);
    ASSERT_EQ(closure->controller_, controller);
    ASSERT_EQ(closure->request_, request);
    ASSERT_EQ(closure->response_, response);

    maid::Channel::default_instance()->CallMethod(method, controller, request, response, closure);

    ASSERT_TRUE(closure->controller_ == NULL);
    ASSERT_TRUE(closure->request_ == NULL);
    ASSERT_TRUE(closure->response_ == NULL);
}

TEST(TcpChannel, TcpChannel)
{
    uv_stream_t* stream = new uv_stream_t();
    maid::TcpChannel channel(stream, NULL);

    ASSERT_EQ(stream, channel.stream());
    ASSERT_EQ(stream->data, &channel);
    ASSERT_EQ(channel.timer_handle().loop, maid_default_loop());
    ASSERT_EQ(channel.timer_handle().data, &channel);
    ASSERT_EQ(channel.idle_handle().loop, maid_default_loop());
    ASSERT_EQ(channel.idle_handle().data, &channel);
}

TEST(TcpChannel, CallMethod)
{
    uv_stream_t* stream = new uv_stream_t();
    maid::TcpChannel channel(stream, NULL);
    TestService_Stub stub(&channel);
    maid::Controller* controller = new maid::Controller();
    Request* request = new Request();
    Response* response = new Response();
    Closure* closure = new Closure(controller, request, response);
    stub.TestMethod(controller, request, response, closure);

    ASSERT_EQ(channel.transmit_id(), 1);
    ASSERT_EQ(channel.async_result().size(), 1u);
    ASSERT_EQ(channel.sending_buffer().size(), 1u);
    ASSERT_FALSE(controller->Failed());

}

TEST(TcpChannel, AfterSendRequest)
{
    uv_stream_t* stream = new uv_stream_t();
    maid::TcpChannel channel(stream, NULL);
    TestService_Stub stub(&channel);
    maid::Controller* controller = new maid::Controller();
    Request* request = new Request();
    Response* response = new Response();
    Closure* closure = new Closure(controller, request, response);
    stub.TestMethod(controller, request, response, closure);

    std::map<uv_write_t*, std::string*>::const_iterator it = channel.sending_buffer().begin();
    uv_write_t* req = (uv_write_t*)it->first;

    maid::TcpChannel::AfterSendRequest(req, 0);
    ASSERT_EQ(channel.sending_buffer().size(), 0u);
}

TEST(TcpChannel, OnRead)
{
    uv_stream_t* stream = new uv_stream_t();
    maid::TcpChannel channel(stream, NULL);
    maid::proto::ControllerProto controller_proto;
    std::string* buffer = maid::WireFormat::Serializer(controller_proto);

    uv_buf_t buf;
    maid::TcpChannel::OnAlloc((uv_handle_t*)stream, buffer->size(), &buf);
    memcpy(buf.base, buffer->data(), buffer->size());
    maid::TcpChannel::OnRead(stream, buffer->size(), &buf);

    ASSERT_EQ(channel.buffer().start + buffer->size(), channel.buffer().end);
}

TEST(TcpChannel, OnIdle)
{
    /*
     * nothing to be done
     */
}

TEST(TcpChannel, OnTimer)
{
    /*
     * nothing to be done
     */
}

TEST(TcpChannel, Handle)
{
    uv_stream_t* stream = new uv_stream_t();
    maid::TcpChannel channel(stream, NULL);
    maid::proto::ControllerProto controller_proto;
    std::string* buffer = maid::WireFormat::Serializer(controller_proto);

    uv_buf_t buf;
    maid::TcpChannel::OnAlloc((uv_handle_t*)stream, buffer->size(), &buf);
    memcpy(buf.base, buffer->data(), buffer->size());
    maid::TcpChannel::OnRead(stream, buffer->size(), &buf);

    int32_t result = channel.Handle();

    ASSERT_EQ(result, 0);
    ASSERT_EQ(channel.buffer().start, channel.buffer().end);
}

TEST(TcpChannel, HandleRequest)
{
    maid::Connector connector;
    uv_stream_t* stream = new uv_stream_t();
    maid::TcpChannel channel(stream, &connector);
    maid::proto::ControllerProto* controller_proto = new maid::proto::ControllerProto();
    controller_proto->set_full_service_name("TestService");
    controller_proto->set_method_name("TestMethod");

    int32_t result = channel.HandleRequest(controller_proto);
    ASSERT_EQ(result, 0);
    /*
     * more
     */
}

TEST(TcpChannel, HandleResponse)
{
    uv_stream_t* stream = new uv_stream_t();
    maid::TcpChannel channel(stream, NULL);
    TestService_Stub stub(&channel);
    maid::Controller* controller = new maid::Controller();
    Request* request = new Request();
    Response* response = new Response();
    Closure* closure = new Closure(controller, request, response);
    stub.TestMethod(controller, request, response, closure);
    maid::proto::ControllerProto* proto = controller->release_proto();

    int32_t result = channel.HandleResponse(proto);
    ASSERT_EQ(result, 0);
    ASSERT_EQ(channel.async_result().size(), 0u);
    ASSERT_EQ(&controller->proto(), proto);
}

TEST(TcpChannel, OnAlloc)
{
    uv_buf_t buf;
    uv_stream_t* stream = new uv_stream_t();
    maid::TcpChannel channel(stream, NULL);
    channel.OnAlloc((uv_handle_t*)stream, 100, &buf);

    ASSERT_TRUE(buf.base != NULL);
    ASSERT_GE(buf.len, 100u);
}

TEST(TcpChannel, Close)
{
    uv_tcp_t* stream = new uv_tcp_t();
    uv_tcp_init(maid_default_loop(), stream);
    maid::TcpChannel channel((uv_stream_t*)stream, NULL);
    maid::Controller* controller = new maid::Controller();
    Request* request = new Request();
    Response* response = new Response();
    Closure* closure = new Closure(controller, request, response);
    TestService_Stub stub(&channel);
    stub.TestMethod(controller, request, response, closure);
    channel.Close();

    ASSERT_EQ(channel.router_controllers().size(), 0u);
    ASSERT_EQ(channel.async_result().size(), 0u);
    ASSERT_TRUE(controller->IsCanceled());
}

TEST(LocalMapRepoChannel, Insert)
{
    maid::LocalMapRepoChannel channel;
    channel.Insert(new TestServiceImpl());
    ASSERT_EQ(channel.service().size(), 1u);
}

TEST(LocalMapRepoChannel, CallMethod)
{
    maid::LocalMapRepoChannel channel;
    channel.Insert(new TestServiceImpl());
    TestService_Stub stub(&channel);

    maid::Controller* controller = new maid::Controller();
    Request* request = new Request();
    request->set_message("test message");
    Response* response = new Response();
    Closure* closure = new Closure(controller, request, response);
    stub.TestMethod(controller, request, response, closure);

    ASSERT_TRUE(request->message()== response->message());
}

TEST(LocalListRepoChannel, Append)
{
    maid::LocalListRepoChannel channel;
    channel.Append(new TestServiceImpl());
    ASSERT_EQ(channel.service().size(), 1u);
}

TEST(LocalListRepoChannel, CallMethod)
{
    maid::LocalMapRepoChannel channel;
    channel.Insert(new TestServiceImpl());
    TestService_Stub stub(&channel);

    maid::Controller* controller = new maid::Controller();
    Request* request = new Request();
    request->set_message("test message");
    Response* response = new Response();
    Closure* closure = new Closure(controller, request, response);
    stub.TestMethod(controller, request, response, closure);

    ASSERT_TRUE(request->message() == response->message());
}
