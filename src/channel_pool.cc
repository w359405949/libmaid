#include "channel_pool.h"
#include "maid/connection.pb.h"
#include "glog/logging.h"
#include "controller.h"
#include "channel.h"

namespace maid {

ChannelPool* ChannelPool::generated_pool_ = NULL;

void InitGeneratedPool()
{
    ChannelPool::generated_pool_ = new ChannelPool();
}

struct StaticInitGeneratedPool
{
    StaticInitGeneratedPool()
    {
        InitGeneratedPool();
    }
} static_init_generated_pool_;

ChannelPool* ChannelPool::generated_pool()
{
    return generated_pool_;
}

ChannelPool::ChannelPool(google::protobuf::RpcChannel* default_channel)
    :default_channel_(default_channel)
{
    gc_.data = this;
    uv_prepare_init(uv_default_loop(), &gc_);
    uv_prepare_start(&gc_, OnGC);
}

ChannelPool::ChannelPool()
    :default_channel_(NULL)
{
    gc_.data = this;
    uv_prepare_init(uv_default_loop(), &gc_);
    uv_prepare_start(&gc_, OnGC);
}

ChannelPool::~ChannelPool()
{
    default_channel_ = NULL;
}

void ChannelPool::OnGC(uv_prepare_t* handle)
{
    ChannelPool* self = (ChannelPool*)handle->data;

    while (!self->channel_invalid_.empty()) {
        delete self->channel_invalid_.top();
        self->channel_invalid_.pop();
    }
}

google::protobuf::RpcChannel* ChannelPool::channel(const google::protobuf::RpcController* rpc_controller) const
{
    const Controller* controller = google::protobuf::down_cast<const Controller*>(rpc_controller);
    const proto::ConnectionProto& connection = controller->proto().GetExtension(proto::connection);
    return channel((google::protobuf::RpcChannel*)connection.id());
}

google::protobuf::RpcChannel* ChannelPool::channel(google::protobuf::RpcChannel* origin) const
{
    std::map<google::protobuf::RpcChannel*, google::protobuf::RpcChannel*>::const_iterator redirect_it;
    redirect_it = redirect_channel_.find(origin);
    if (redirect_channel_.end() != redirect_it) {
        origin = redirect_it->second;
    }

    std::map<google::protobuf::RpcChannel*, google::protobuf::RpcChannel*>::const_iterator channel_it;
    channel_it = channel_.find(origin);
    if (channel_.end() != channel_it) {
        return origin;
    }

    return default_channel();
}

google::protobuf::RpcChannel* ChannelPool::channel() const
{
    std::map<google::protobuf::RpcChannel*, google::protobuf::RpcChannel*>::const_iterator channel_it;
    channel_it = channel_.begin();
    if (channel_it != channel_.end()) {
        return channel_it->second;
    }
    return default_channel();
}

google::protobuf::RpcChannel* ChannelPool::default_channel() const
{
    return default_channel_ == NULL ? Channel::default_instance() : default_channel_;
}

void ChannelPool::AddChannel(google::protobuf::RpcChannel* channel)
{
    CHECK(channel_.find(channel) == channel_.end());
    channel_[channel] = channel;
}

void ChannelPool::RemoveChannel(google::protobuf::RpcChannel* channel)
{
    std::map<google::protobuf::RpcChannel*, google::protobuf::RpcChannel*>::iterator it;
    it = channel_.find(channel);
    if (channel_.end() != it) {
        channel_.erase(channel);
        channel_invalid_.push(channel);
    }
}

void ChannelPool::ChannelRedirect(google::protobuf::RpcChannel* another, google::protobuf::RpcChannel* origin, bool force)
{
    std::map<google::protobuf::RpcChannel*, google::protobuf::RpcChannel*>::iterator it;
    it = redirect_channel_.find(origin);
    if (redirect_channel_.end() == it || force) {
        redirect_channel_[origin] = another;
    }
}

void ChannelPool::RemoveRedirect(google::protobuf::RpcChannel* channel)
{
    redirect_channel_.erase(channel);
}

void ChannelPool::Close()
{
    redirect_channel_.clear();
    channel_.clear();
    default_channel_ = NULL;

    OnGC(&gc_);
    uv_prepare_stop(&gc_);

}

}
