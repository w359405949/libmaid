#ifndef ECHO_CHANNEL_H
#define ECHO_CHANNEL_H
#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include <ev.h>

class Channel : public google::protobuf::RpcChannel
{
public:
    Channel();
    virtual void CallMethod(const google::protobuf::MethodDescriptor *emthod,
                            google::protobuf::RpcController *controller,
                            const google::protobuf::Message *request,
                            google::protobuf::Message *response,
                            google::protobuf::Closure *done);

    virtual ~Channel();
    void Update();
    void ListenOn(std::string host, std::port);

private:
    static void onRecevied(EV_P_ ev_io *w, int revents);
    void CheckConnection();
    int32_t Handle(int8_t *buffer, int32_t start, int32_t end);
private:
    struct ev_loop *m_loop;
    struct ev_io *m_read_watcher;
    int32_t m_fd;
    std::string m_host;
    int32_t m_port;
    int8_t *m_buffer;
    int32_t m_buffer_pending;
    int32_t m_buffer_max;
    int32_t m_header_length;
};


#endif /*CHANNEL_H*/
