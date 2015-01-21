#include "maid/channel.h"
#include "channelimpl.h"

using maid::Channel;

Channel::Channel(uv_loop_t* loop)
{
    channel_ = new ChannelImpl(loop);
}

void Channel::CallMethod(const google::protobuf::MethodDescriptor* method,
        google::protobuf::RpcController* controller,
        const google::protobuf::Message* request,
        google::protobuf::Message* response,
        google::protobuf::Closure* done)
{
    channel_->CallMethod(method, controller, request, response, done);
}

int64_t Channel::Listen(const char* host, int32_t port, int32_t backlog)
{
    return channel_->Listen(host, port, backlog);
}

int64_t Channel::Connect(const char* host, int32_t port, bool as_default)
{
    return channel_->Connect(host, port, as_default);
}

void Channel::AppendService(google::protobuf::Service* service)
{
    channel_->AppendService(service);
}

void Channel::AppendConnectionMiddleware(maid::proto::ConnectionMiddleware* middleware)
{
    channel_->AppendConnectionMiddleware(middleware);
}

void Channel::set_default_connection_id(int64_t connection_id)
{
    channel_->set_default_connection_id(connection_id);
}

int64_t Channel::default_connection_id()
{
    return channel_->default_connection_id();
}

void Channel::Update()
{
    channel_->Update();
}

void Channel::ServeForever()
{
    channel_->ServeForever();
}

Channel::~Channel()
{
    delete channel_;
}

uv_loop_t* Channel::loop()
{
    return channel_->loop();
}
