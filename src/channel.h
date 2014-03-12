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

    /*
     * add a Listen Address
     * return
     * < 0: error happend, may be errno(will perror), may be -1.
     * > 0: fd. check only, NEVER do any operation(read/write/close .etc) on it.
     */
    int32_t Listen(const std::string& host, const int32_t port, const int32_t backlog=1);

    /*
     * add a Peer Address
     * return
     * < 0: error happend, may be errno(will perror), may be -1.
     * > 0: fd. check only, NEVER do any operation(read/write/close .etc) on it.
     */
    int32_t Connect(const std::string& host, const int32_t port); // return fd for visit, but do not do any operation directly.
    int32_t AppendService(const google::protobuf::Service* service);
    const google::protobuf::Service* GetServiceByName(const char * name) const;
    const google::protobuf:;Service* GetServiceByName(const std::string& name) const;

private:
    static void OnRead(EV_P_ ev_io* w, int32_t revents);
    static void OnWrite(EV_P_ ev_io* w, int32_t revents);
    static void OnAccept(EV_P_ ev_io* w, int32_t revents);
    static void OnConnect(EV_P_ ev_io* w, int32_t revents);

private
    static int32_t Realloc(void** ptr, uint32_t* origin_size, const uint32_t new_size, const size_t type_size);

private:
    void CloseConnection(const int32_t fd);
    void Handle(const int32_t fd);
    int32_t HandleRequest(const int32_t fd, const maid::proto::ControllerMeta& stub_meta,
            const int8_t* message_start, const int32_t message_length);
    int32_t HandleResponse(const int32_t fd, const maid::proto::ControllerMeta& meta,
            const int8_t* message_start, const int32_t message_length);
    bool IsEffictive(const int32_t fd) const;
    bool IsConnected(const int32_t fd);
    bool SetNonBlock(const int32_t fd);


private:
    // service
    int32_t google::protobuf::Service * service_;
    int32_t service_current_size_;
    int32_t service_max_size_;

private:
    // libev
    struct ev_loop* loop_;
    struct ev_io* read_watcher_;
    struct ev_io* write_watcher_;
    uint32_t io_watcher_max_size_;

    struct ev_io* accept_watcher_;
    uint32_t accept_watcher_max_size_;

    struct ev_io* connect_watcher_;
    uint32_t connect_watcher_max_size_;

private:
    // packet
    const uint32_t header_length_;

    // read buffer
    int8_t ** buffer_;
    uint32_t * buffer_pending_index_;
    uint32_t * buffer_max_length_; // expend double size
    uint32_t buffer_list_max_size_; // expend double size

    // stub
    Context ** stub_context_;
    uint32_t stub_context_list_max_size_;

    // non-stub
    Context ** context_;
    uint32_t context_list_max_size_;

};

} /* namespace channel */

} /* namespace maid */

#endif /*MAID_CHANNEL_H*/
