#include "gtest/gtest.h"
#include "channel_factory.h"
#include "channel.h"
#include "controller.h"

UV_EXTERN int uv_tcp_bind(uv_tcp_t* handle,
                          const struct sockaddr* addr,
                          unsigned int flags)
{
    return 0;
}


UV_EXTERN int uv_listen(uv_stream_t* stream, int backlog, uv_connection_cb cb)
{
    return 0;
}

UV_EXTERN int uv_accept(uv_stream_t* server, uv_stream_t* client)
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


UV_EXTERN void uv_close(uv_handle_t* handle, uv_close_cb close_cb)
{
    close_cb(handle);
}


TEST(Acceptor, Acceptor)
{
    maid::Acceptor* acceptor = new maid::Acceptor();
    maid::Controller controller;

    ASSERT_TRUE(acceptor->handle() == NULL);
    ASSERT_EQ(acceptor->router_channel(), maid::Channel::default_instance());
    ASSERT_EQ(acceptor->middleware_channel(), maid::Channel::default_instance());
}

TEST(Acceptor, Listen)
{
    maid::Acceptor acceptor;
    int32_t result = acceptor.Listen("0.0.0.0", 5555);

    ASSERT_EQ(result, 0);
    ASSERT_TRUE(acceptor.handle() != NULL);
}

TEST(Acceptor, Connected)
{
    maid::Acceptor acceptor;
    acceptor.Listen("0.0.0.0", 5555);
    uv_stream_t* stream = new uv_stream_t();
    maid::TcpChannel* channel = new maid::TcpChannel(stream, NULL);
    acceptor.Connected(channel);
    ASSERT_EQ(acceptor.channel_size(), 1u);
}

TEST(Acceptor, Disconnected)
{
    maid::Acceptor acceptor;
    acceptor.Listen("0.0.0.0", 5555);
    uv_stream_t* stream = new uv_stream_t();
    maid::TcpChannel* channel = new maid::TcpChannel(stream, NULL);
    acceptor.Connected(channel);
    acceptor.Disconnected(channel);
    ASSERT_EQ(acceptor.channel_size(), 0u);
}

TEST(Acceptor, OnAccept)
{
    maid::Acceptor acceptor;
    acceptor.Listen("0.0.0.0", 5555);

    acceptor.OnAccept((uv_stream_t*)acceptor.handle(), 0);
    ASSERT_EQ(acceptor.channel_size(), 1u);
}

TEST(Connector, Connector)
{
    maid::Connector connector(NULL, NULL, NULL);
    maid::Controller controller;

    ASSERT_TRUE(connector.req() == NULL);
}

TEST(Connector, OnConnect)
{
    maid::Connector connector(NULL, NULL, NULL);
    connector.Connect("0.0.0.0", 5555);
    uv_connect_t* req = new uv_connect_t();
    uv_stream_t* stream = new uv_stream_t();
    req->handle = stream;
    req->data = &connector;
    maid::Connector::OnConnect(req, 0);

    ASSERT_TRUE(connector.req() == NULL);
}

TEST(Connector, Connect)
{
    maid::Connector connector(NULL, NULL, NULL);
    connector.Connect("0.0.0.0", 5555);

    ASSERT_TRUE(connector.req() != NULL);
}

TEST(Connector, Connected)
{
    maid::Connector connector(NULL, NULL, NULL);
    uv_stream_t* stream = new uv_stream_t();
    maid::TcpChannel* channel = new maid::TcpChannel(stream, NULL);
    connector.Connected(channel);

}

TEST(Connector, Disconnected)
{
    maid::Connector connector(NULL, NULL, NULL);
    uv_stream_t* stream = new uv_stream_t();
    maid::TcpChannel* channel = new maid::TcpChannel(stream, NULL);
    connector.Connected(channel);
    connector.Disconnected(channel);

}
