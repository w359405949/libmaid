#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include "channel.h"

Channel::Channel(std::string host, int32_t port)
    :m_fd(-1),
    m_read_watcher(NULL),
    m_host(host),
    m_port(port),
    m_buffer_pending(0),
    m_buffer_max(10000),
    m_header_length(8),
    m_loop(EV_DEFAULT)
{
    m_buffer = (int8_t*)malloc(m_buffer_max);
    memset(m_buffer, 0, m_buffer_max);
}

void Channel::CallMethod(const google::protobuf::MethodDescriptor * method,
                            google::protobuf::RpcController * controller,
                            const google::protobuf::Message * request,
                            google::protobuf::Message * response,
                            google::protobuf::Closure * done)
{
    CheckConnection();
    int32_t count = ::write(m_fd, method->name().c_str(), method->name().length());
    printf("send:%d, expect:%ld\n", count, method->name().length());
}

void Channel::OnRecevied(EV_P_ ev_io *w, int revents)
{
    Channel *self = (Channel*)w->data;
    int count = ::read(w->fd, self->m_buffer, self->m_buffer_max - self->m_buffer_pending);
    if(count < 0){
        ::puts("disconnect\n");
        self->m_buffer_pending = 0;
        self->m_fd = -1;
        ev_io_stop(EV_A_ w);
        //delete w;
        return;
    }

    self->m_buffer_pending += count;
    int32_t start = 0;
    while(self->m_buffer_pending - start >= self->m_header_length){
        int32_t handled = self->Handle(self->m_buffer, start, self->m_buffer_pending);
        start += handled;
    }
    ::printf("start:%d, pending:%d\n", start, self->m_buffer_pending);
    assert(start <= self->m_buffer_pending && "overflowed");
    ::memmove(self->m_buffer + start, self->m_buffer, self->m_buffer_pending - start);
    self->m_buffer_pending -= start;
}

int32_t Channel::Handle(int8_t* buffer, int32_t start, int32_t end)
{
    int32_t controller_length = 0;
    int32_t message_length = 0;
    memcpy(buffer, &controller_length, 4);
    memcpy(buffer, &message_length, 4);
    //int32_t count = ::write(m_fd, buffer + start, end - start);
    //printf("send:%d, expected:%d", count, end-start);
    printf("recevied:%s\n", buffer);
    return start - end;
}

void Channel::CheckConnection()
{
    if(m_fd < 0){
        struct sockaddr_in sock_addr;
        sock_addr.sin_addr.s_addr = inet_addr(m_host.c_str());
        sock_addr.sin_family = AF_INET;
        sock_addr.sin_port = htons(m_port);

        int32_t sock_fd = ::socket(AF_INET, SOCK_STREAM, 0);
        int32_t result = ::connect(sock_fd, (struct sockaddr*)&sock_addr, sizeof(sock_addr));
        if(result < 0){
            printf("connect failed\n");
            m_fd = result;
        }else{
            m_fd = sock_fd;
        }
        m_read_watcher = new ev_io;
        m_read_watcher->data = this;
        ev_io_init(m_read_watcher, OnRecevied, m_fd, EV_READ);
        ev_io_start(m_loop, m_read_watcher);
    }
}

Channel::~Channel()
{
    if(m_read_watcher != NULL){
        delete m_read_watcher;
    }
    m_read_watcher = NULL;
    delete m_buffer;
    m_loop = NULL;
}

void Channel::Update()
{
    ev_run(m_loop, EVRUN_NOWAIT);
}
