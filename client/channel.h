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
    void CloseConnection(int32_t fd);
    void Handle(int32_t fd);

private:
    // libev loop
    struct ev_loop* loop_;
    struct ev_io* read_watcher_;
    struct ev_io* write_watcher_;
    struct ev_prepare prepare_;
    struct ev_check check_;

    int32_t io_watcher_max_size_;

    // packet
    const int32_t header_length_;

    // read buffer
    int8_t ** buffer_;
    int32_t * buffer_pending_index_;
    int32_t * buffer_max_length_; // expend double size
    int32_t buffer_max_size_; // expend double size

    //
    Context ** pending_send_packets_;
};

} /* namespace channel */

} /* namespace maid */

#endif /*MAID_CHANNEL_H*/
