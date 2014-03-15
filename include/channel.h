#ifndef MAID_CHANNEL_H
#define MAID_CHANNEL_H
#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include <ev.h>

namespace maid
{

namespace proto
{
class ControllerMeta;
}

namespace controller
{
class Controller;
}

namespace channel
{

class Context
{
public:
    //google::protobuf::MethodDescriptor* method_;
    maid::controller::Controller* controller_;
    google::protobuf::Message* request_;
    google::protobuf::Message* response_;
    google::protobuf::Closure* done_;
    struct Context* next_;

    Context(struct ev_loop* loop);
    void Destroy();
    static void OnGC(struct ev_loop* loop, ev_check* w, int32_t revents);
    void Ref();
private:
    struct ev_loop* loop_;
    struct ev_check gc_;
    int32_t ref_;
};

class Channel : public google::protobuf::RpcChannel
{
public:
    Channel(struct ev_loop* loop);
    virtual void CallMethod(const google::protobuf::MethodDescriptor * method,
                            google::protobuf::RpcController *controller,
                            const google::protobuf::Message *request,
                            google::protobuf::Message *response,
                            google::protobuf::Closure *done);

    virtual ~Channel();

    /*
     * add a Listen/Connect Address
     * return
     * < 0: error happend, may be errno(will perror), may be -1.
     * > 0: fd. check only, NEVER do any operation(read/write/close .etc) on it.
     */
    int32_t Listen(const std::string& host, int32_t port, int32_t backlog=1);
    int32_t Connect(const std::string& host, int32_t port);

    /*
     * service for remote request
     */
    int32_t AppendService(google::protobuf::Service* service);

    /*
     * context for send.
     * return
     * 0: success.
     * -1: failed. invalid fd.
     */
    int32_t AppendContext(int32_t fd, Context* context); // send

private:
    static void OnRead(EV_P_ ev_io* w, int32_t revents);
    static void OnWrite(EV_P_ ev_io* w, int32_t revents);
    static void OnAccept(EV_P_ ev_io* w, int32_t revents);
    static void OnConnect(EV_P_ ev_io* w, int32_t revents);

private:
    static int32_t Realloc(void** ptr, uint32_t* origin_size, uint32_t new_size, uint32_t type_size);

    static bool SetNonBlock(int32_t fd);

private:
    void CloseConnection(int32_t fd);
    void Handle(int32_t fd);
    int32_t HandleRequest(int32_t fd,
            maid::proto::ControllerMeta& stub_meta,
            const int8_t* message_start, int32_t message_length);
    int32_t HandleResponse(int32_t fd,
            maid::proto::ControllerMeta& meta,
            const int8_t* message_start, int32_t message_length);
    bool IsEffictive(int32_t fd) const;
    bool IsConnected(int32_t fd) const;
    google::protobuf::Service* GetServiceByName(const std::string& name);
    int32_t NewConnection(int32_t fd);

private:
    // service
    google::protobuf::Service ** service_;
    uint32_t service_current_size_;
    uint32_t service_max_size_;

private:
    // libev
    struct ev_loop* loop_;
    struct ev_io** read_watcher_; // level 1: fd; level 2: watcher point;
    struct ev_io** write_watcher_; // level 1: fd; level 2: watcher_point;
    uint32_t io_watcher_max_size_;

    struct ev_io** accept_watcher_; // level 1: fd; level 2: watcher point;
    uint32_t accept_watcher_max_size_;

    struct ev_io** connect_watcher_; // level 1 fd; level 2: watcher point;
    uint32_t connect_watcher_max_size_;

private:
    // packet
    const uint32_t header_length_;

    // read buffer
    int8_t ** buffer_; // level 1: fd; level 2: buffer start
    uint32_t * buffer_pending_index_; // level 1: fd
    uint32_t * buffer_max_length_; // level 1: fd
    uint32_t buffer_list_max_size_;

    //
    Context** context_; // level 1: transmit id
    uint32_t context_list_max_size_;

    //
    Context** write_pending_; // level 1: fd;
    uint32_t write_pending_list_max_size_; //

};

} /* namespace channel */

} /* namespace maid */

#endif /*MAID_CHANNEL_H*/
