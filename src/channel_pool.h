#pragma once
#include <map>
#include <stack>

#include <google/protobuf/service.h>
#include <uv.h>

namespace maid {

class ChannelPool
{
public:
    static ChannelPool* generated_pool();

    ChannelPool(google::protobuf::RpcChannel* default_channel);
    ChannelPool();
    virtual ~ChannelPool();

    /*
     * default_channel, never return NULL.
     */
    google::protobuf::RpcChannel* default_channel() const;

    /*
     * check channel if exist (return itself) or default_channel or redirect one.
     */
    google::protobuf::RpcChannel* channel(google::protobuf::RpcChannel* origin) const;

    /*
     * get channel by controller, it is more use full in google::protobuf::Service implement.
     *
     */
    google::protobuf::RpcChannel* channel(const google::protobuf::RpcController* controller) const;

    /*
     * get first valid channel
     *
     */
    google::protobuf::RpcChannel* channel() const;

    /*
     * add new channel;
     */
    void AddChannel(google::protobuf::RpcChannel* channel);

    /*
     * remove channel and delete it by auto-gc.
     */
    void RemoveChannel(google::protobuf::RpcChannel* channel);

    /*
     * redirect channel from origin to another.
     * override exist rule if force = true.
     */
    void ChannelRedirect(google::protobuf::RpcChannel* another, google::protobuf::RpcChannel* origin, bool force);

    /*
     * remove redirect rule
     */
    void RemoveRedirect(google::protobuf::RpcChannel* channel);

    /*
     *
     */
    void Close();

private:
    static void OnGC(uv_prepare_t* handle);

private:
    friend void InitGeneratedPool();

    std::map<google::protobuf::RpcChannel*, google::protobuf::RpcChannel*> channel_;
    std::map<google::protobuf::RpcChannel*, google::protobuf::RpcChannel*> redirect_channel_;
    std::stack<google::protobuf::RpcChannel*> channel_invalid_;
    google::protobuf::RpcChannel* default_channel_;

    uv_prepare_t gc_;

    static ChannelPool* generated_pool_;

    GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(ChannelPool);
};

}
