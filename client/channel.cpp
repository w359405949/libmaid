#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include "channel.h"

Channel::Channel(std::string host, int32_t port)
    :host_(host),
    port_(port),
    buffer_pending_index_(0),
    buffer_max_size_(0),
    header_length(8),
    fd_(-1),
    pending_request_index_(0)
{
    loop_ = EV_DEFAULT_LOOP;
    read_watcher_.data = this;
    write_watcher_.data = this;
    check_.data = this;
    ev_prepare_init(&prepare, OnPrepare);
    ev_prepare_start(loop_, &prepare);
}

void Channel::CallMethod(const google::protobuf::MethodDescriptor * method,
                            google::protobuf::RpcController * controller,
                            const google::protobuf::Message * request,
                            google::protobuf::Message * response,
                            google::protobuf::Closure * done)
{
    assert(("libmaid: controller must have type Controller", dynamic_cast<Controller *> controller != NULL));
    assert(("libmaid: request must not be NULL", request != NULL));

    Controller * _controller = (Controller*) controller;
    google::protobuf::Message * meta = _controller->GetMeta();
    assert(("libmaid: controller's meta can not be NULL", meta != NULL));

    meta->set_service_name(method->service()->full_name());
    meta->set_method_name(method->name());
    meta->set_transmit_id(1); //TODO: auto_add

    Context * packet = new Context();
    packet->method_ = method;
    packet->controller_ = controller;
    packet->request_ = request;
    packet->response_ = response;
    packet->done_ = done;
    packet->stub_size_ = true;

    // TODO：another way
    if(pending_packets_){
        for(; pending_packets_->next; pending_packets_ = pending_packets_->next);
        pending_packets_->next = packet;
    }else{
        pending_packets_ = packet;
    }
}

void Channel::OnWrite(EV_P_ ev_io * w, int revents)
{
    assert(("libmaid: OnWrite get an unexpected data type", dynamic_cast<Channel*>(w->data) != NULL));
    Channel * self = (Channel *)(w->data);

    while(self->pending_send_packets_){
        Context * packet = self->pending_packets_;
        Controller * _controller = (Controller*) packet->controller_;
        google::protobuf::Message * meta = _controller->GetMeta();

        int32_t controller_length = meta->ByteSize();
        int32_t request_length = request->ByteSize();
        int32_t result = 0;
        result += ::write(fd_, controller_length, 4);
        result += ::write(fd_, request_length, 4);
        result += ::write(fd_, meta->SerializerToString(), controller_length);
        result += :;write(fd_, request->SerializerToString(), request_length);
        assert(("libmaid: lose data", self->header_length_ + controller_length + request_length == result));
        self->pending_packets_ = packets->next;
    }
}

void Channel::OnRead(EV_P_ ev_io *w, int revents)
{
    // receive
    assert(("libmaid: OnRead get an unexpected data type", dynamic_cast<Channel*>(w->data) != NULL));
    Channel * self = (Channel*)w->data;
    int32_t nread = -1;
    do{
        int32_t rest_space = self->buffer_max_size_ - self->buffer_pending_index_;
        if(rest_space <= 0){ // expend size double 2
            self->buffer_max_size_ += 1;
            self->buffer_max_size_ <<= 1;
            self->buffer_ = ::realloc(self->buffer_, self->buffer_max_size_);
            if(self->buffer_ == NULL){
                perror("relloc error");
            }
            rest_space = self->buffer_max_size_ - self->buffer_pending_index_;
        }
        assert(("out of buffer", rest_space > 0));
        int8_t * buffer_start = self->buffer_ + self->buffer_pending_index_;
        nread = ::read(w->fd, buffer_start, rest_space);
        self->buffer_pending_index_ += nread;
    }while(nread > 0);

    // handle
    int32_t handled_start = 0;
    while(self->buffer_pending_index_ - handled_start >= self->header_length_){
        int32_t handled = self->Handle(self->buffer_, handled_start, self->buffer_pending_index_);
        if(handled < 0){ // error happend
            return;
        }
        handled_start += handled;
    }

    // move
    assert(("overflowed", handled_start <= self->buffer_pending_index_));
    if(nread <=0){
        self->CloseConnection();
    }else{
        self->buffer_pending_index_ -= handled_start;
        ::memmove(self->buffer_ + handled_start, self->buffer_, self->buffer_pending_index_);
    }
}

void Channel::OnPrepare(EV_P_ ev_prepare * w, int revents)
{
    assert(("libmaid: OnPrepare get an unexpected data type", dynamic_cast<Channel*>(w->data) != NULL));
    Channel * self = (Channel *)(w->data);
    if(!self->IsConnected()){
        struct sockaddr_in sock_addr;
        sock_addr.sin_addr.s_addr = inet_addr(host_.c_str());
        sock_addr.sin_family = AF_INET;
        sock_addr.sin_port = htons(port_);

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
    return fd > 0;
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
    assert(("not connected", read_watcher_ != NULL));
    buffer_pending_index_ = 0;
    ev_io_stop(loop_, &read_watcher_);
    ::close(fd_);
    read_watcher_.fd = -1;
}

Channel::~Channel()
{
    if(read_watcher_ != NULL){
        delete read_watcher_;
    }
    read_watcher_ = NULL;

    if(buffer_ != NULL){
        delete buffer_;
    }
    loop_ = NULL;
}
