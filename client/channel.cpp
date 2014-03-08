#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <ev.h>
#include "channel.h"
#include "controller.h"

using maid::channel::Channel;
using maid::controller::Controller;

Channel::Channel(struct ev_loop* loop, std::string host, int32_t port)
    :host_(host),
    port_(port),
    buffer_pending_index_(0),
    buffer_max_size_(0),
    header_length_(8),
    fd_(-1)
{
    loop_ = loop;
    read_watcher_.data = this;
    write_watcher_.data = this;
    prepare.data = this;
    check.data = this;
    ev_prepare_init(&prepare_, OnPrepare);
    ev_prepare_start(loop_, &prepare);
    ev_check_init(&check_, OnCheck);
    ev_check_start(loop_, &check_);
}

void Channel::CallMethod(const google::protobuf::MethodDescriptor * method,
                            google::protobuf::RpcController * controller,
                            const google::protobuf::Message * request,
                            google::protobuf::Message * response,
                            google::protobuf::Closure * done)
{
    assert(("libmaid: controller must have type Controller", dynamic_cast<Controller *>(controller) != NULL));
    assert(("libmaid: request must not be NULL", request != NULL));

    Controller * _controller = (Controller*) controller;
    maid::proto::Controller& meta = _controller->get_meta_data();

    meta.set_service_name(method->service()->full_name());
    meta.set_method_name(method->name());
    meta.set_transmit_id(1); //TODO: auto_add
    meta.set_stub_side(true);

    Context * packet = new Context();
    packet->method_ = method;
    packet->controller_ = controller;
    packet->request_ = request;
    packet->response_ = response;
    packet->done_ = done;

    // TODO：another way
    if(pending_send_packets_){
        for(; pending_send_packets_->next; pending_send_packets_ = pending_send_packets_->next);
        pending_send_packets_->next = packet;
    }else{
        pending_send_packets_ = packet;
    }
}

void Channel::OnWrite(EV_P_ ev_io * w, int revents)
{
    Channel * self = (Channel *)(w->data);

    while(self->pending_send_packets_){
        Context * packet = self->pending_send_packets_;
        Controller * _controller = (Controller*)packet->controller_;
        maid::proto::Controller meta = _controller->get_meta_data();

        assert(("packet->request cat not be NULL", packet->request_ != NULL));
        std::string controller_buffer;
        meta.SerializeToString(&controller_buffer);
        std::string request_buffer;
        packet->request_->SerializeToString(&request_buffer);

        uint32_t controller_nl = htonl(controller_buffer.length());
        uint32_t request_nl = htonl(request_buffer.length());
        int32_t result = 0;
        result += ::write(self->fd_, (const void *)&controller_nl, sizeof(controller_nl));
        result += ::write(self->fd_, (const void *)&request_nl, sizeof(request_nl));
        result += ::write(self->fd_, controller_buffer.c_str(), controller_buffer.length());
        result += ::write(self->fd_, request_buffer.c_str(), request_buffer.length());
        assert(("libmaid: lose data", self->header_length_ + controller_buffer.length() + request_buffer.length() == result));
        self->pending_send_packets_ = packet->next;
    }
}

void Channel::OnRead(EV_P_ ev_io *w, int revents)
{
    // receive
    Channel * self = (Channel*)(w->data);
    while(w->fd > self->buffer_max_size_){
        self->buffer_max_size_ += 1;
        self->buffer_max_size_  <<= 2;

        self->buffer_pending_index_ = (int32_t *)realloc(self->buffer_pending_index_, self->buffer_max_size_ * sizeof(int32_t));
        self->buffer_max_length = (int32_t *)realloc(self->buffer_max_length_, self->buffer_max_size_ * sizeof(int32_t));
        self->buffer_ = (int8_t **)realloc(self->buffer_, self->buffer_max_size_ * sizeof(int8_t));


        self->buffer_pending_index_[w->fd] = 0;
        self->buffer_max_length_[w->fd] = 0;
        self->buffer_[w->fd] = NULL;
    }

    int32_t buffer_pending_index = self->buffer_pending_index_[w->fd];
    int32_t buffer_max_length = self->buffer_max_length_[w->fd];

    int32_t nread = -1;
    do{
        int32_t rest_space = buffer_max_size__ - buffer_pending_index_;
        if(rest_space <= 0){ // expend size double 2
            self->buffer_max_size_ += 1;
            self->buffer_max_size_ <<= 1;
            self->buffer_ = (int8_t*)::realloc((void*)self->buffer_, self->buffer_max_size_);
            if(self->buffer_ == NULL){
                perror("realloc:");
            }
            rest_space = self->buffer_max_size_ - self->buffer_pending_index_;
        }
        assert(("out of buffer", rest_space > 0));
        int8_t * buffer_start = self->buffer_ + self->buffer_pending_index_;
        nread = ::read(w->fd, buffer_start, rest_space);
        if nread
        self->buffer_pending_index_ += nread;
    }while(nread > 0);

    self->Handle(w->fd);
}

void Channel::OnCheck(EV_P_ ev_check * w, int revents)
{
    Channel * self = (Channel *)(w->data);

    // handle
    int32_t handled_start = 0;
    while(self->buffer_pending_index_ - handled_start >= self->header_length_){
        int32_t controller_length = 0;
        int32_t message_length = 0;
        memcpy(self->buffer_, &controller_length, 4);
        memcpy(self->buffer_, &message_length, 4);
        handled_start += self->buffer_pending_index;
    }

    // move
    assert(("overflowed", handled_start <= self->buffer_pending_index_));
    self->buffer_pending_index_ -= handled_start;
    ::memmove(self->buffer_ + handled_start, self->buffer_, self->buffer_pending_index_);
}

void Channel::OnPrepare(EV_P_ ev_prepare * w, int revents)
{
    Channel * self = (Channel *)(w->data);
    if(!self->IsConnected()){
        struct sockaddr_in sock_addr;
        sock_addr.sin_addr.s_addr = inet_addr(self->host_.c_str());
        sock_addr.sin_family = AF_INET;
        sock_addr.sin_port = htons(self->port_);

        int32_t sock_fd = ::socket(AF_INET, SOCK_STREAM, 0);
        if(sock_fd < 0){
            perror("socket:");
            return;
        }
        int32_t result = ::connect(sock_fd, (struct sockaddr*)&sock_addr, sizeof(sock_addr));
        if(result < 0){
            perror("connect:");
            return;
        }

        self->fd_ = sock_fd;
        ev_io_init(&(self->read_watcher_), OnRead, sock_fd, EV_READ);
        ev_io_start(self->loop_, &(self->read_watcher_));
        ev_io_init(&(self->write_watcher_), OnWrite, sock_fd, EV_WRITE);
        ev_io_start(self->loop_, &(self->write_watcher_));
    }
}

bool Channel::IsConnected() const
{
    return fd_ > 0;
}

int32_t Channel::Handle(int8_t* buffer, int32_t start, int32_t end)
{
    int32_t controller_length = 0;
    int32_t message_length = 0;
    memcpy(buffer, &controller_length, 4);
    memcpy(buffer, &message_length, 4);
    return start - end;
}

void Channel::CloseConnection()
{
    assert(("not connected", fd_ > 0));
    buffer_pending_index_ = 0;
    ev_io_stop(loop_, &read_watcher_);
    ev_io_stop(loop_, &write_watcher_);
    ::close(fd_);
    fd_ = -1;
}

Channel::~Channel()
{
    if(buffer_ != NULL){
        delete buffer_;
    }
    loop_ = NULL;
}
