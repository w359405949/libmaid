#ifndef MAID_CHANNEL_H
#define MAID_CHANNEL_H
#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include <ev.h>

namespace maid
{
namespace channel
{

struct Context
{
    const google::protobuf::MethodDescriptor * method_;
    const google::protobuf::RpcController * controller_;
    const google::protobuf::Message * request_;
    const google::protobuf::Message * response_;
    const google::protobuf::Closure * done_;
    struct Context * next;
};

class Channel : public google::protobuf::RpcChannel
{
public:
    Channel(struct ev_loop* loop, std::string host, int32_t port);
    virtual void CallMethod(const google::protobuf::MethodDescriptor * method,
                            google::protobuf::RpcController *controller,
                            const google::protobuf::Message *request,
                            google::protobuf::Message *response,
                            google::protobuf::Closure *done);

    virtual ~Channel();

private:
    static void OnRead(EV_P_ ev_io * w, int revents);
    static void OnWrite(EV_P_ ev_io * w, int revents);
    static void OnPrepare(EV_P_ ev_prepare * w, int revents); // Connect and Reconnect

private:
    bool IsConnected() const;
    int32_t Handle(int8_t *buffer, int32_t start, int32_t end);
    void CloseConnection();

private:
    // libev loop
    struct ev_loop * loop_;
    struct ev_io read_watcher_;
    struct ev_io write_watcher_;
    struct ev_prepare prepare;

    //
    std::string host_;
    int32_t port_;
    int32_t fd_;

    // packet
    int32_t header_length_;

    // read buffer
    int8_t *buffer_;
    int32_t buffer_pending_index_;
    int32_t buffer_max_size_;

    //
    Context * pending_send_packets_;
};

} /* namespace channel */

} /* namespace maid */

#endif /*MAID_CHANNEL_H*/
