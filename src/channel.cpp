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
using maid:proto::ControllerMeta;

Channel::Channel(struct ev_loop* loop)
    :loop_(loop),
    read_watcher(NULL),
    write_watcher(NULL),
    io_watcher_max_size_(0),
    header_length_(8)
    buffer_(NULL),
    buffer_pending_index_(NULL),
    buffer_max_length_(NULL),
    buffer_list_max_size_(0),
    packet_(NULL),
    packet_list_max_size_(0)
{
}

Channel::~Channel()
{
    if(NULL != buffer_){
        for(int32_t i = 0; i < buffer_list_max_size_; ++i){
            if(NULL != buffer_[i]){
                ::free(buffer_[i]);
                buffer_[i] = NULL;
            }
        }
        ::free(buffer_);
    }
    if(NULL != buffer_pending_index_){
        ::free(buffer_pending_index_):
    }
    if(NULL != buffer_max_length_){
        ::free(buffer_max_length_);
    }
    if(0 <  io_watcher_max_size_){
        for(int32_t fd = 0; fd < io_watcher_max_size_; ++fd){
            CloseConnection(fd);
        ::free(read_watcher_);
        ::free(write_watcher_);
    }
}

int32_t AppendService(google::protobuf::Service* service)
{
    assert(("service can not be NULL", NULL != service));
    int32_t result = Realloc((void**)&service_, &service_max_size_,
            service_current_size_, sizeof(google::protobuff::Service);
    if(0 > result){
        return -1;
    }
    service_[service_current_size_++] = service;
    return 0;
}

/*
 * wether a 'done' is required need more ''.
 */
void Channel::CallMethod(const google::protobuf::MethodDescriptor * method,
        google::protobuf::RpcController * controller,
        const google::protobuf::Message * request,
        google::protobuf::Message * response,
        google::protobuf::Closure * done)
{
    assert(("libmaid: controller must have type Controller", NULL != dynamic_cast<Controller *>(controller)));
    assert(("libmaid: request must not be NULL", NULL != request));
    assert(("libmaid: done can not be NULL", NULL != done));
    if(!meta.wide() && !IsEffictive(fd){
        controller->setFaield("invalid fd");
        done->Run();
        return;
    }

    Context * context = (Context*)malloc(sizeof(Context));
    if(NULL == context){
        std::string error_text(strerror(errno));
        controller->SetFailed(error_text);
        done->Run();
        return;
    }
    context->method_ = method;
    context->controller_ = controller;
    context->request_ = request;
    context->response_ = response;
    context->done_ = done;

    /*
     *  assume the oldest id always handled first.
     *  so simple transmit_id search WOULD NOT ALWAYS BAD as O(n).
     *  TODO: more test and benchmark.
     */
    uint32_t transmit_id = packet_list_max_size_;
    for(uint32_t i = 0; i < packet_list_max_size_; ++i) {
        if(NULL != packet_[i]){
            transmit_id = i;
            break;
        }
    }

    /*
     * extend size double.
     */
    int32_t result = Realloc(&packet_, &packet_list_max_size_, transmit_id);
    if(0 > result){
        std::string error_text(strerror(errno));
        controller->SetFailed(error_text);
        ::free(context);
        done->Run();
        return;
    }

    Controller * _controller = (Controller*) controller;
    ControllerMeta& meta = _controller->get_meta_data();

    meta.set_service_name(method->service()->full_name());
    meta.set_method_name(method->name());
    meta.set_transmit_id(transmit_id);
    meta.set_stub_side(true);

    packet_[transmit_id] = context;

    /*
     * TODO: another way
     */
    if(meta.wide()){
        /*
         * BroadCast to all connected fd
         */
        for(int32_t fd = 0; fd < context_list_max_size; ++fd) {
            if(!IsEffictive(fd)){
                continue;
            }
            int32_t ref = meta.ref();
            meta.set_ref(++ref);
            Context * head = context_[fd];
            if(NULL != head){
                for(; head->next; head = head->next);
                head->next = context;
            }else{
                head = context;
            }
            context_[fd] = head;
        }
        if(meta.ref() == 0){
            controller->setFaield("no effective connection");
            ::free(context);
            done->Run();
        }
        return;
    }

    int32_t fd = meta.fd; /* has been checked */
    Context * head = context_[fd];
    if(NULL != head){
        for(; head->next; head = head->next);
        head->next = context;
    }else{
        head = context;
    }
    context_[fd] = head;
}

bool Channel::IsEffictive(int32_t fd) const
{
    return fd < io_watcher_max_size && NULL != write_watcher[fd] && NULL != read_wache[fd];
}

void Channel::OnWrite(EV_P_ ev_io * w, int revents)
{
    Channel* self = (Channel*)(w->data);
    Context* context = self->context_[w->fd];

    while(context){
        Controller * controller = (Controller*)context->controller_;
        ControllerMeta meta = controller->get_meta_data();

        /*
         * serializer can be cached if neccessary
         */
        std::string controller_buffer;
        uint32_t controller_nl = htonl(controller_buffer.length());
        meta.SerializeToString(&controller_buffer);

        std::string message_buffer;
        if(meta.stub()){
            context->request_->SerializeToString(message_buffer);
        }else{
            context->response_->SerializeToString(message_buffer);
        }

        uint32_t message_nl = htonl(message_buffer.length());
        int32_t result = 0;
        result += ::write(w->fd, (const void *)&controller_nl, sizeof(controller_nl));
        if(0 > result){
            std::string error_text(strerror(errno));
            _controller.SetFailed(error_text);
            context->done_->Run();
            self->CloseConnection(w->fd);
            return;
        }
        result += ::write(w->fd, (const void *)&message_nl, sizeof(message_nl));
        if(0 > result){
            std::string error_text(strerror(errno));
            _controller.SetFailed(error_text);
            context->done_->Run();
            self->CloseConnection(w->fd);
            return;
        }

        //if(!meta.SerializeToFileDescriptor(w->fd)){}
        result += ::write(w->fd, meta..c_str(), controller_buffer.length());
        if(0 > result){
            std::string error_text(strerror(errno));
            _controller.SetFailed(error_text);
            context->done_->Run();
            self->CloseConnection(w->fd);
            return;
        }

        result += ::write(self->fd_, message_buffer.c_str(), message_buffer.length());
        if(0 > result){
            std::string error_text(strerror(errno));
            _controller.SetFailed(error_text);
            context->done_->Run();
            self->CloseConnection(w->fd);
            return;
        }

        if(!meta.stub()){
            /*
             * TODO:response. clean unused info, like context.
             */
        }

        assert(("libmaid: lose data", self->header_length_ + controller_buffer.length() + request_buffer.length() == result));
        context = context->next;
    }
}

void Channel::OnRead(EV_P_ ev_io *w, int revents)
{
    Channel * self = (Channel*)(w->data);

    /*
     * extend size
     */
    uint32_t buffer_list_max_size = 0;
    int32_t result = -1;

    buffer_list_max_size = self->buffer_list_max_size_;
    result = Realloc((void**)&(self->buffer_pending_index_),
            &buffer_list_max_size, w->fd, sizeof(uint32_t));
    if(0 > result){
        self->CloseConnection(w->fd);
        return;
    }
    buffer_list_max_size = self->buffer_list_max_size_;
    result = Realloc((void**)&(self->buffer_pending_index_),
            &buffer_list_max_size, w->fd, sizeof(uint32_t));
    if(0 > result){
        self->CloseConnection(w->fd);
        return;
    }
    buffer_list_max_size = self->buffer_list_max_size_;
    result = Realloc((void**)&(self->buffer_),
            &buffer_list_max_size, w->fd, sizeof(int8_t*));
    if(0 > result){
        self->CloseConnection(w->fd);
        return;
    }
    self->buffer_list_max_size_ = buffer_list_max_size;

    /*
     * read
     */
    int32_t result = Realloc((void**)&)(self->buffer_[w->fd]),
            &(self->buffer_max_length_[w->fd]),
            self->buffer_pending_index_[w->fd], typeof(int8_t));
    if(0 > result){
        CloseConnection(w->fd);
        perror("realloc:");
        return;
    }

    int8_t* buffer_start = self->buffer_[w->fd] + self->buffer_pending_index_[w->fd];
    int32_t rest_space = self->buffer_max_length_[w->fd] - self->buffer_pending_index_[w->fd];
    int32_t nread = ::read(w->fd, buffer_start, rest_space);
    if(nread <= 0){
        if(errno != EAGAIN && errno != EWOULDBLOCK || nread == 0){
            self->CloseConnection(w->fd);
            return;
        }
    }
    self->buffer_pending_index_[w->fd] += nread;
    /*
     * handle
     */
    if(self->buffer_pending_index_[w->fd] > self->header_length_){
        self->Handle(w->fd);
    }
}

void Channel::Handle(int32_t fd)
{
    uint32_t buffer_pending_index = buffer_pending_index_[fd];
    uint32_t buffer_max_length = buffer_max_length_[fd];
    int8_t* buffer = buffer_[fd];

    // handle
    int32_t handled_start = 0;
    while(buffer_pending_index - handled_start >= header_length){
        /*
         * in use of protobuf, this block can be fixed.
         *
         */
        int32_t controller_length_nl = 0;
        int32_t message_length_nl = 0;
        memcpy(buffer, &controller_length, 4);
        memcpy(buffer, &message_length, 4);
        int32_t controller_length = ntohl(controller_length_nl);
        int32_t message_length = ntohl(message_length_nl);
        /*
         * TODO:check length valid and overflow
         */
        int32_t total_length = header_length_ + controller_length + message_length;
        if(total_length < buffer_pending_index - handled_start){
            break; // lack data
        }
        ControllerMeta meta;
        if(!meta.ParseFromArray(buffer + handled_start + header_length_, controller_length)){
            CloseConnection(fd); // unknown meta, can not recovered
            return;
        }
        int8_t* message_start = buffer + handled_start + header_length_, controller_length;
        if(0 == message_length){
            handled_start += header_length_ + controller_length;
        }
        int32_t handled = -1;
        if(meta.stub()){
            handled = HandleRequest(fd, meta, message_start, message_length);
        }else{
            handled = HandleResponse(fd, meta, message_start, message_length);
        }
        if(0 > handled){
            CloseConnection(fd);
            return;
        }
        if(0 == handled){//delay
            break;
        }
        handled_start += handled;
    }

    // move
    assert(("overflowed", handled_start <= buffer_pending_index));
    buffer_pending_index -= handled_start;
    ::memmove(buffer + handled_start, buffer, buffer_pending_index);

    buffer_pending_index_[w->fd] = buffer_pending_index;
    buffer_max_length_[w->fd] = buffer_max_length;
    buffer_[w->fd] = buffer;
}

int32_t HandleResponse(const int32_t fd, const ControllerMeta& meta,
        const int8_t* message_start, const int32_t message_length)
{
    if(meta.transmit_id < connect_watcher_max_size_){
        /*
         * TODO: do something while context is lost, log?
         */
        return message_length;
    }
    Context* context = context_[meta.transmit_id];
    if(NULL == context){
        /*
         * TODO: do something while context is lost, log?
         */
        return message_length;
    }
    Controller* controller = context->controller_;
    if(NULL == controller){
        /*
         * TODO: should not happend
         */
        return message_length;
    }
    google::protobuf::Message* response = context->response_;
    if(NULL == response){
        /*
         * TODO: should not happen
         */
        return message_length;
    }

    ControllerMeta& stub_meta = controller->get_meta();
    if(stub_meta.wide()){
        controller->Unref();
    }
    if(!meta.Failed()){
        google::protobuf::Message * new_response = context->response_->New();
        if(NULL == new_response){
            return 0; //delay
        }
        if(!new_response->ParseFromArray(buffer, message_length)){
            return -1;//can not recovered
        }
        response->MergeFrom(new_response);
        delete new_response;
    }

    if(!stub_meta.wide() || !stub_meta.get_ref())
        context->done_->Run();
        ::free(context);
        context_[meta.transmit_id] = NULL;
    }
}

int32_t HandleRequest(const int32_t fd, const ControllerMeta& stub_meta,
        const int8_t* message_start, const int32_t message_length)
{
    Context* context = new Context();
    Controller* controller = new Controller();

    /*
     * TODO: check
     */

    context->controller_ = controller;
    controller->get_meta_data().CopyFrom(stub_meta);
    controller->get_meta_data().set_stub(false);
    const google::protobuf::Service* service = GetServiceByName(stub_meta.method_name());
    if(NULL == service){
        controller->SetFailed("no such service");
    }
    const google::protobuf::ServiceDescriptor* service_descriptor = serivce->GetDescriptor();
    const google::protobuf::MethodDescriptor* method_descriptor = service_descriptor->FindMethodByName(stub_meta.method_name());
    service->CallMethod(method_descriptor, controller, request, response, done);

}

int32_t Channel::Connect(std::string& host, int32_t port)
{
    int32_t fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if(0 > fd){
        perror("socket:");
        return fd;
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    int32_t result = inet_pton(AF_INET, host.c_str(), &addr.sin_addr);
    if(0 > result){
        perror("inet_pton:");
        return result;
    }

    SetNonBlock(fd);
    int32_t result = ::connect(fd, (struct sockaddr *)&addr, sizeof(addr));
    if(0 > result) {
        if(errno == EINPROGRESS){
            uint32_t connect_watcher_max_size = connect_watcher_max_size_;
            uint32_t original_size = connect_watcher_max_size;
            while(fd > connect_watcher_max_size){
                connect_watcher_max_size += 1;
                connect_watcher_max_size <<= 1;
                struct ev_io* new_connect_wacher = ::realloc(connect_watcher_,
                        connect_watcher_max_size);
                if(NULL == new_connect_wacher){
                    perror("realloc:");
                    ::close(fd);
                    return -1;
                }
                connect_watcher_ = new_read_watcher;
            }
            if(
            struct ev_io* connect_watcher = (ev_io *)malloc(sizeof(ev_io));
            if(NULL == connect_watcher){
                ::close(fd);
                return -1;
            }
            ev_io_init(connect_watcher, OnConnect, fd, EV_READ);
            connect_watcher->data = this;
            ev_io_start(loop_, connect_watcher);
        }else{
            perror("connect:");
            return result;
        }
    }else{
        /*
         * watcher_size <= the real watcher size is safe.
         */
        while(fd > io_watcher_max_size_){
            io_watcher_max_size_ += 1;
            io_watcher_max_size_ <<= 1;
            struct ev_io* new_read_watcher = (ev_io *)::realloc(read_watcher_,
                    io_watcher_max_size_ * sizeof(ev_io));
            if(NULL == new_read_watcher){
                perror("realloc:");
                CloseConnection(fd);
                return -1;
            }
            read_watcher_ = new_read_watcher;

            struct ev_io* new_write_watcher = (ev_io *)::realloc(write_watcher_,
                    io_watcher_max_size_ * sizeof(ev_io));
            if(NULL == new_write_watcher){
                perror("realloc:");
                CloseConnection(fd);
                return -1;
            }
            write_watcer_ = new_write_watcher;
        }
        ev_io_init(&read_watcher_[fd], OnRead, fd, EV_READ);
        ev_io_start(loop_, &read_watcher_[fd]);
        ev_io_init(&write_watcer_[fd], OnWrite, fd, EV_WRITE);
        ev_io_start(loop_, &write_watcher_[fd]);
    }
    return fd;
}

int32_t Channel::Listen(std::string& host, int32_t port, int32_t backlog)
{
    int32_t fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if(fd <= 0){
        perror("socket:");
        return fd;
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    int32_t result = inet_pton(AF_INET, host.c_str(), &addr.sin_addr);
    if(result < 0){
        perror("inet_pton:");
        return result;
    }

    int32_t result = ::bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    if(result < 0){
        perror("bind:");
        ::close(fd);
        return result;
    }
    int32_t result = ::listen(fd, backlog);
    if(result < 0){
        perror("listen:");
        ::close(fd);
        return result;
    }

    uint32_t accept_watcher_max_size = accept_watcher_max_size_;
    while(fd > accept_watcher_max_size){
        io_watcher_max_size_ += 1;
        io_watcher_max_size_ <<= 1;
        struct ev_io* new_accept_watcher = (ev_io *)::realloc(accept_watcher_,
                accept_watcher_max_size * sizeof(ev_io));
        if(NULL == new_accept_watcher){
            perror("realloc:");
            ::close(fd);
            return -1;
        }
        accept_watcher_ = new_accept_watcher;
    }
    if(accept_watcher_max_size_ < accept_watcher_max_size){
        uint32_t esize = accept_watcher_max_size_ - accept_watcher_max_size;
        ::memset(new_accept_watcher + accept_watcher_max_size_ , 0, esize);
    }

    struct ev_io* accept_watcher = (ev_io *)malloc(sizeof(ev_io));
    if(NULL == accept_watcher){
        perror("malloc:");
        ::close(fd);
        return -1;
    }
    ev_io_init(accept_watcher, OnAccept, fd, EV_READ);
    ev_io_start(loop_, accept_watcher);
    return fd;
}

void Channel::OnConnect(EV_P_ ev_io* w, int revents)
{
    Channel* self = (Channel*)(w->data);
    int32_t result = ::conenct(w->fd, );
    if(0 > result && EINPROGRESS == errno){
        return;
    ev_io_stop(EV_A_ w);
    if(0 == result){ // connected
        ev_io_init(w, OnRead, w->fd, EV_READ);
        ev_io_start(EV_A_ w);
    }else{
        ::free(w);
    }
}

void Channel::OnAccept(EV_P_ ev_io* w, int revents)
{
    do{
        struct sockaddr_in addr;
        int32_t fd = ::accept(w->fd, (struct sockaddr *)&addr, sizeof(addr));
        if(0 > fd&& (EAGAIN == errno|| EWOULDBLOCK == errno)){
            break;
        }
        int32_t non_block = SetNonBlock(fd);
        if(!non_block){
            ::close(fd);
            continue;
        }
        if(fd > io_watcher_max_size_){
            io_watcher_max_size_ += 1;
            io_watcher_max_size_ <<= 1;
            read_watcher_ = (ev_io *)::realloc(read_watcher_, io_watcher_max_size_ * sizeof(ev_io));
            write_watcher_ = (ev_io *)::realloc(write_watcher_, io_watcher_max_size_ * sizeof(ev_io));
            /*
             * TODO:check
             */
        }
        ev_io_init(&read_watcher_[fd], OnRead, fd, EV_READ);
        ev_io_start(EV_A_ &read_watcher_[fd]);
        ev_io_init(&write_watcer_[fd], OnWrite, fd, EV_WRITE);
        ev_io_start(EV_A_ &write_watcher_[fd]);
    }while(1);
}

void Channel::CloseConnection(int32_t fd)
{
    //
    ::close(fd);

    // libev
    if(fd < io_watcher_max_size_){
        ev_io_stop(loop_, &read_watcher_[fd]);
        ev_io_stop(loop_, &write_watcher_[fd]);
        read_watcher_[fd].fd = -1;
        write_watcher_[fd].fd = -1;
    }
    if(fd < accept_watcher_max_size_){
        ev_io_stop(loop_, &accept_watcher_[fd]);
    }
    if(fd < connect_watcher_max_size_){
        ev_io_stop(loop_, &connect_watcher_[fd]);
    }
    //ev_break(loop_, EVBREAK_ONE); // do not call the stoped event

    // read buffer
    buffer_pending_index_[fd] = 0;

    // write buffer
    if(fd < context_list_max_size_){
        Context * context = context_[fd];
        while(context){
            ::free(context);
            context = context->next_;
        }
    }
}

bool Channel::SetNonBlock(int32_t fd) const
{
    int32_t flags = ::fcntl(fd, F_GETFL);
    if(0 > flags){
        perror("fcntl:");
        return false;
    }else{
        flags |= O_NONBLOCK;
        if(0 > ::fcntl(fd, F_SETFL, flags)){
            perror("fcntl:");
            return false;
        }
    }
    return true;
}

/*
 * extend *ptr double size utils large than expected_size. and initial new add space as 0.
 * UGLY IMPLEMENT, DO NOT REPLACE realloc AT OTHER PLACE.
 *
 * parameters
 * ptr: changed every time call realloc(if realloc changed), can not be NULL. but (*ptr) can be NULL(realloc allowed).
 * origin_size: changed ONLY LARGE THAN expected size, can not be NULL. origin_size <= the real (*ptr) size is safe when error happend.
 *
 * returns
 * 0: SUCCESS.
 * -1: FAILED. (realloc failed).
 */
int32_t Channel::Realloc(void** ptr, uint32_t* origin_size, const uint32_t expect_size, const size_t type_size)
{
    /*
     *
     */
    if(NULL == ptr || NULL == origin_size){
        ::puts("libmaid: ptr and origin_size can not be NULL in Realloc");
        return -1;
    }

    uint32_t size = *origin_size;
    while(size  < expect_size){
        size += 1;
        size <<= 1;
        void* new_ptr = ::realloc(*ptr, size);
        if(NULL == new_ptr){
            ::perror("Realloc:");
            return -1;
        }
        *ptr = new_ptr;
    }
    ::memset(*ptr + *origin_size, 0, size - *origin_size);
    *origin_size = size;
    return 0;
}
