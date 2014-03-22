#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <ev.h>
#include "channel.h"
#include "controller.h"
#include "closure.h"

using maid::channel::Channel;
using maid::controller::Controller;
using maid::proto::ControllerMeta;
using maid::closure::RemoteClosure;

Channel::Channel(struct ev_loop* loop)
    :service_(NULL),
    service_current_size_(0),
    service_max_size_(0),

    loop_(loop),
    read_watcher_(NULL),
    write_watcher_(NULL),
    io_watcher_max_size_(0),

    accept_watcher_(NULL),
    accept_watcher_max_size_(0),

    connect_watcher_(NULL),
    connect_watcher_max_size_(0),

    header_length_(8),

    buffer_(NULL),
    buffer_pending_index_(NULL),
    buffer_max_length_(NULL),
    buffer_list_max_size_(0),

    controller_(NULL),
    controller_list_max_size_(0),

    write_pending_(NULL),
    write_pending_list_max_size_(0)
{
    assert(("libmaid: loop can not be none", NULL != loop));
    signal(SIGPIPE, SIG_IGN);
}

Channel::~Channel()
{
    if(NULL != buffer_){
        for(uint32_t i = 0; i < buffer_list_max_size_; ++i){
            if(NULL != buffer_[i]){
                ::free(buffer_[i]);
                buffer_[i] = NULL;
            }
        }
        ::free(buffer_);
    }
    if(NULL != buffer_pending_index_){
        ::free(buffer_pending_index_);
    }
    if(NULL != buffer_max_length_){
        ::free(buffer_max_length_);
    }
    for(uint32_t fd = 0; fd < io_watcher_max_size_; ++fd){
        CloseConnection(fd);
    }
    if(0 <  io_watcher_max_size_){
        ::free(read_watcher_);
        ::free(write_watcher_);
    }
}

int32_t Channel::AppendService(google::protobuf::Service* service)
{
    if(NULL == service){
        assert(("libmaid: service can not be NULL", NULL != service));
        return -1;
    }

    int32_t result = Realloc((void**)&service_, &service_max_size_,
            service_current_size_, sizeof(google::protobuf::Service*));
    if(0 > result){
        return -1;
    }
    service_[service_current_size_++] = service;
    return 0;
}

void Channel::CallMethod(const google::protobuf::MethodDescriptor* method,
        google::protobuf::RpcController* _controller,
        const google::protobuf::Message* request,
        google::protobuf::Message* response,
        google::protobuf::Closure* done)
{
    assert(("libmaid: controller must have type maid::controller::Controller", NULL != dynamic_cast<Controller *>(_controller)));
    assert(("libmaid: request must not be NULL", NULL != request));
    assert(("libmaid: done can not be NULL", NULL != done));

    Controller* controller = (Controller*)_controller;
    controller->Ref();

    controller->set_request((google::protobuf::Message*)request);
    controller->set_response(response);
    controller->set_done(done);

    int32_t result = RegistController(controller);
    if(0 > result){
        std::string error_text(strerror(errno));
        controller->SetFailed(error_text);
        done->Run();
        return;
    }

    ControllerMeta& meta = controller->meta_data();
    meta.set_service_name(method->service()->full_name());
    meta.set_method_name(method->name());
    meta.set_stub(true);

    result = PushController(controller->fd(), controller);
    if(-1 == result){
        controller->SetFailed("invalid fd");
        done->Run();
        UnregistController(controller->fd(), meta);
        return;
    }else if(-2 == result){
        /* retry if needed */
        controller->SetFailed("channel busy");
        done->Run();
        UnregistController(controller->fd(), meta);
        return;
    }
}

int32_t Channel::RegistController(Controller* controller)
{
    /*
     *  assume the oldest id always handled first.
     *  so simple transmit_id search WOULD NOT ALWAYS BAD as O(n).
     *  TODO: more test and benchmark.
     */
    uint32_t transmit_id = controller_list_max_size_;
    for(uint32_t i = 0; i < controller_list_max_size_; ++i) {
        if(NULL == controller_[i]){
            transmit_id = i;
            break;
        }
    }
    int32_t result = Realloc((void**)&controller_, &controller_list_max_size_,
            transmit_id, sizeof(Controller*));
    if(0 > result){
        return -1;
    }
    controller->meta_data().set_transmit_id(transmit_id);

    controller->Ref();
    controller_[transmit_id] = controller;
    return 0;
}

Controller* Channel::UnregistController(int32_t fd, ControllerMeta& meta)
{
    Controller* r_controller = controller_[meta.transmit_id()];
    if(NULL == r_controller){
        assert(("libmaid: controller not regist", false));
        return NULL;
    }
    if(r_controller->fd() != fd){
        assert(("libmaid: not the same controller", false));
        return NULL;
    }

    controller_[meta.transmit_id()] = NULL;
    r_controller->Destroy();
    return r_controller;
}

int32_t Channel::PushController(int32_t fd, Controller* controller)
{
    if(NULL == controller){
        assert(("libmaid: controller should not be NULL", false));
        return -1;
    }

    if(!IsEffictive(fd)){
        return -1;
    }
    int32_t result = Realloc((void**)&write_pending_,
            &write_pending_list_max_size_, fd, sizeof(Controller*));
    if(0 > result){
        return -2; // dealy if needed
    }

    // Do not need to check head.
    Controller* head = controller;
    head->set_next(write_pending_[fd]);

    for(; head->next(); head = head->next());

    head->set_next(controller);
    controller->set_next(NULL);
    write_pending_[fd] = head;
    controller->Ref();
    if(controller == head){
        struct ev_io* write_watcher = write_watcher_[fd];
        ev_io_init(write_watcher, OnWrite, fd, EV_WRITE);
        ev_io_start(loop_, write_watcher);
    }
    return 0;
}

Controller* Channel::FrontController(int32_t fd)
{
    if(fd >= write_pending_list_max_size_){
        return NULL;
    }
    Controller* head = write_pending_[fd];
    if(NULL == head){
        return head;
    }
    write_pending_[fd] = head->next();
    head->Destroy();
    return head;
}

bool Channel::IsEffictive(int32_t fd) const
{
    return 0 < fd && fd < io_watcher_max_size_ && NULL != write_watcher_[fd] && NULL != read_watcher_[fd];
}

void Channel::OnWrite(EV_P_ ev_io * w, int revents)
{
    Channel* self = (Channel*)(w->data);
    if(w->fd > self->write_pending_list_max_size_){
        return;
    }
    Controller* controller = NULL;
    while(1){
        controller = self->FrontController(w->fd);
        if(NULL == controller){
            ev_io_stop(EV_A_ w);
            return;
        }

        /*
         * serializer can be cached if neccessary
         */
        ControllerMeta& meta = controller->meta_data();
        uint32_t controller_nl = ::htonl(meta.ByteSize());
        int32_t result = -1;
        int32_t message_nl = 0;
        int32_t nwrite = 0;

        if(controller->Failed()){
            message_nl = ::htonl(0);
        }else{
            if(meta.stub()){
                message_nl = ::htonl(controller->request()->ByteSize());
            }else{
                message_nl = ::htonl(controller->response()->ByteSize());
            }
        }

        result = ::write(w->fd, &controller_nl, sizeof(controller_nl));
        if(0 > result){
            if(meta.stub()){
                std::string error_text(strerror(errno));
                controller->SetFailed(error_text);
                controller->done()->Run();
            }
            self->CloseConnection(w->fd);
            return;
        }
        nwrite += result;

        result = ::write(w->fd, &message_nl, sizeof(message_nl));
        if(0 > result){
            if(meta.stub()){
                std::string error_text(strerror(errno));
                controller->SetFailed(error_text);
                controller->done()->Run();
            }
            self->CloseConnection(w->fd);
            return;
        }
        nwrite += result;

        if(!meta.SerializeToFileDescriptor(w->fd)){
            if(meta.stub()){
                std::string error_text(strerror(errno));
                controller->SetFailed(error_text);
                controller->done()->Run();
            }
            self->CloseConnection(w->fd);
            return;
        }

        if(!controller->Failed()){
            bool serialize = false;
            if(meta.stub()){
                serialize = controller->request()->SerializeToFileDescriptor(w->fd);
            }else{
                serialize = controller->response()->SerializeToFileDescriptor(w->fd);
            }
            if(!serialize){
                if(meta.stub()){
                    std::string error_text(strerror(errno));
                    controller->SetFailed(error_text);
                    controller->done()->Run();
                }
                self->CloseConnection(w->fd);
                return;
            }
        }
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
    result = Realloc((void**)&self->buffer_pending_index_,
            &buffer_list_max_size, w->fd, sizeof(uint32_t));
    if(0 > result){
        self->CloseConnection(w->fd);
        return;
    }
    buffer_list_max_size = self->buffer_list_max_size_;
    result = Realloc((void**)&self->buffer_max_length_,
            &buffer_list_max_size, w->fd, sizeof(uint32_t));
    if(0 > result){
        self->CloseConnection(w->fd);
        return;
    }
    buffer_list_max_size = self->buffer_list_max_size_;
    result = Realloc((void**)&self->buffer_, &buffer_list_max_size, w->fd,
            sizeof(int8_t*));
    if(0 > result){
        self->CloseConnection(w->fd);
        return;
    }
    self->buffer_list_max_size_ = buffer_list_max_size;

    result = Realloc((void**)&(self->buffer_[w->fd]),
            &(self->buffer_max_length_[w->fd]),
            self->buffer_pending_index_[w->fd], sizeof(int8_t));
    if(0 > result){
        self->CloseConnection(w->fd);
        return;
    }

    /*
     * read
     */
    int8_t* buffer_start = self->buffer_[w->fd] + self->buffer_pending_index_[w->fd];
    int32_t rest_space = self->buffer_max_length_[w->fd] - self->buffer_pending_index_[w->fd];
    int32_t nread = ::read(w->fd, buffer_start, rest_space);
    if(0 >= nread){
        if((EAGAIN != errno && EWOULDBLOCK != errno) || 0 == nread){
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
    while(buffer_pending_index - handled_start >= header_length_){
        /*
         * in use of protobuf, this block can be fixed.
         *
         */
        int32_t controller_length_nl = 0;
        int32_t message_length_nl = 0;
        memcpy(&controller_length_nl, buffer, 4);
        memcpy(&message_length_nl, buffer + 4, 4);
        int32_t controller_length = ntohl(controller_length_nl);
        int32_t message_length = ntohl(message_length_nl);
        /*
         * TODO:check length valid and overflow
         */
        int32_t total_length = header_length_ + controller_length + message_length;
        if(buffer_pending_index - handled_start < total_length){
            break; // lack data
        }

        ControllerMeta meta;
        if(!meta.ParseFromArray(buffer + handled_start + header_length_, controller_length)){
            CloseConnection(fd); // unknown meta, can not recovered
            return;
        }
        int8_t* message_start = buffer + handled_start + header_length_ + controller_length;

        int32_t result = -1;
        if(meta.stub()){
            result = HandleRequest(fd, meta, message_start, message_length);
        }else{
            result = HandleResponse(fd, meta, message_start, message_length);
        }
        if(-1 == result){
            CloseConnection(fd);
            return;
        }
        if(-2 == result){//delay
            break;
        }
        handled_start += header_length_ + controller_length + message_length;
    }

    // move
    assert(("libmaid: overflowed", handled_start <= buffer_pending_index));
    buffer_pending_index -= handled_start;
    ::memmove(buffer, buffer + handled_start, buffer_pending_index);

    buffer_pending_index_[fd] = buffer_pending_index;
    buffer_max_length_[fd] = buffer_max_length;
    buffer_[fd] = buffer;
}

int32_t Channel::HandleResponse(int32_t fd, ControllerMeta& meta,
        const int8_t* message_start, int32_t message_length)
{
    Controller* controller = UnregistController(fd, meta);
    if(NULL == controller){
        /*
         * TODO: do something while controller is lost, log?
         */
        return -1;
    }
    controller->set_fd(fd);
    controller->meta_data().CopyFrom(meta);
    controller->meta_data().set_stub(true);

    if(!controller->Failed()){
        google::protobuf::Message* response = controller->response();
        google::protobuf::Message* new_response = response->New();
        if(NULL == new_response){
            return -2; //delay
        }
        if(!new_response->ParseFromArray(message_start, message_length)){
            return -1;//can not recovered
        }
        response->MergeFrom(*new_response);
        delete new_response;
    }

    controller->done()->Run();
    controller->Destroy(); // Unref
    return 0;
}

int32_t Channel::HandleRequest(int32_t fd, ControllerMeta& stub_meta,
        const int8_t* message_start, int32_t message_length)
{
    Controller* controller= new Controller(loop_);
    if(NULL == controller){
        return -2; // delay
    }

    google::protobuf::Closure* done = new RemoteClosure(loop_, this, controller);
    if(NULL == done){
        controller->Destroy();
        return -2;
    }
    controller->set_done(done);
    controller->set_fd(fd);

    ControllerMeta& meta = controller->meta_data();
    meta.CopyFrom(stub_meta);
    meta.set_stub(false);
    google::protobuf::Service* service = GetServiceByName(meta.service_name());
    if(NULL == service){
        controller->SetFailed("service not exist");
        done->Run();
        controller->Destroy();
        return 0;
    }
    const google::protobuf::ServiceDescriptor* service_descriptor = service->GetDescriptor();
    const google::protobuf::MethodDescriptor* method = service_descriptor->FindMethodByName(meta.method_name());
    if(NULL == method){
        controller->SetFailed("method not exist");
        done->Run();
        controller->Destroy();
        return 0;
    }

    google::protobuf::Message* request = service->GetRequestPrototype(method).New();
    if(NULL == request){
        controller->Destroy();
        return -2; // delay
    }
    controller->set_request(request);

    if(!request->ParseFromArray(message_start, message_length)){
        controller->Destroy();
        return -1;
    }
    google::protobuf::Message* response = service->GetResponsePrototype(method).New();
    if(NULL == response){
        controller->Destroy();
        return -2; // delay
    }
    controller->set_response(response);

    service->CallMethod(method, controller, request, response, done);
    return 0;
}

int32_t Channel::Connect(const std::string& host, int32_t port)
{
    int32_t fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if(0 > fd){
        perror("socket:");
        return fd;
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    int32_t result = -1;
    result = ::inet_pton(AF_INET, host.c_str(), &addr.sin_addr);
    if(0 > result){
        perror("inet_pton:");
        return result;
    }

    result = ::connect(fd, (struct sockaddr *)&addr, sizeof(addr));
    if(0 == result) {
        NewConnection(fd);
        return fd;
    }
    if(errno != EINPROGRESS){
        perror("connect:");
        return result;
    }

    result = Realloc((void**)&connect_watcher_, &connect_watcher_max_size_,
            fd, sizeof(struct ev_io*));
    if(0 > result){
        CloseConnection(fd);
        return -1;
    }
    struct ev_io* connect_watcher = (struct ev_io*)malloc(sizeof(struct ev_io));
    if(NULL == connect_watcher){
        perror("malloc:");
        CloseConnection(fd);
        return -1;
    }

    connect_watcher->data = this;
    connect_watcher->fd = fd;
    connect_watcher_[fd] = connect_watcher;
    ev_io_init(connect_watcher, OnConnect, fd, EV_READ);
    ev_io_start(loop_, connect_watcher);
    return fd;
}

int32_t Channel::Listen(const std::string& host, int32_t port, int32_t backlog)
{
    int32_t fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if(fd <= 0){
        perror("socket:");
        return fd;
    }
    bool non_block = SetNonBlock(fd); // no care
    if(!non_block){
        perror("realloc:");
        CloseConnection(fd);
        return -1;
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    int32_t result = -1;
    result = ::inet_pton(AF_INET, host.c_str(), &addr.sin_addr);
    if(0 > result){
        perror("inet_pton:");
        return result;
    }
    result = ::bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    if(0 > result){
        perror("bind:");
        CloseConnection(fd);
        return result;
    }
    result = ::listen(fd, backlog);
    if(0 > result){
        perror("listen:");
        CloseConnection(fd);
        return result;
    }

    result = Realloc((void**)&accept_watcher_, &accept_watcher_max_size_, fd,
            sizeof(struct ev_io*));
    if(0 > result){
        perror("realloc:");
        CloseConnection(fd);
        return -1;
    }

    struct ev_io* accept_watcher = (struct ev_io*)malloc(sizeof(struct ev_io));
    if(NULL == accept_watcher){
        CloseConnection(fd);
        return -1;
    }

    accept_watcher->data = this;
    accept_watcher_[fd] = accept_watcher;
    accept_watcher->fd = fd;
    ev_io_init(accept_watcher, OnAccept, fd, EV_READ);
    ev_io_start(loop_, accept_watcher);
    return fd;
}

void Channel::OnConnect(EV_P_ ev_io* w, int revents)
{
    Channel* self = (Channel*)(w->data);
    struct sockaddr_in addr;
    socklen_t len;
    ::getpeername(w->fd, (struct sockaddr*)&addr, &len);
    int32_t result = ::connect(w->fd, (struct sockaddr*)&addr, sizeof(addr));
    if(0 > result && EINPROGRESS == errno){
        return;
    }
    ev_io_stop(EV_A_ w);
    result = self->NewConnection(w->fd);
    if(0 > result){
        self->CloseConnection(w->fd);
    }
    ::free(w);
}

void Channel::OnAccept(EV_P_ ev_io* w, int revents)
{
    Channel* self = (Channel*)w->data;
    while(1){
        struct sockaddr_in addr;
        socklen_t len;
        int32_t fd = ::accept(w->fd, (struct sockaddr *)&addr, &len);
        if(0 > fd && (EAGAIN == errno|| EWOULDBLOCK == errno)){
            break;
        }

        int32_t result = -1;
        result = self->NewConnection(fd);
        if(0 > result){
            self->CloseConnection(fd);
        }
    }
}

void Channel::CloseConnection(int32_t fd)
{
    //printf("close connection:%d\n", fd);
    //
    ::close(fd);

    // read buffer
    if(fd < buffer_list_max_size_){
        buffer_pending_index_[fd] = 0;
    }

    // write buffer
    bool empty = true;
    do{
        Controller* controller = FrontController(fd);
        if(NULL == controller){
            break;
        }
        empty = false;
        if(controller->meta_data().stub()){
            controller->SetFailed("connection closed");
            controller->done()->Run();
            UnregistController(fd, controller->meta_data());
        }
        controller->Destroy();
    }while(1);

    // libev
    if(fd < io_watcher_max_size_){
        struct ev_io* read_watcher = read_watcher_[fd];
        if(NULL != read_watcher){
            ev_io_stop(loop_, read_watcher);
            ::free(read_watcher);
        }
        read_watcher_[fd] = NULL;

        struct ev_io* write_watcher = write_watcher_[fd];
        if(NULL != write_watcher){
            if(!empty){
                ev_io_stop(loop_, write_watcher);
            }
            ::free(write_watcher);
        }
        write_watcher_[fd] = NULL;
    }
    if(fd < accept_watcher_max_size_){
        struct ev_io* accept_watcher = accept_watcher_[fd];
        if(NULL != accept_watcher){
            ev_io_stop(loop_, accept_watcher);
            ::free(accept_watcher);
        }
        accept_watcher_[fd] = NULL;
    }
    if(fd < connect_watcher_max_size_){
        struct ev_io* connect_watcher = connect_watcher_[fd];
        if(NULL != connect_watcher){
            ev_io_stop(loop_, connect_watcher);
            ::free(connect_watcher);
        }
        connect_watcher_[fd] = NULL;
    }

}

bool Channel::SetNonBlock(int32_t fd)
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

int32_t Channel::NewConnection(int32_t fd)
{
    int32_t result = -1;
    result = SetNonBlock(fd);
    if(0 > result){
        CloseConnection(fd);
        return -1;
    }

    uint32_t io_watcher_max_size = 0;
    io_watcher_max_size = io_watcher_max_size_;
    result = Realloc((void**)&read_watcher_, &io_watcher_max_size, fd,
            sizeof(struct ev_io*));
    if(0 > result){
        perror("realloc:");
        CloseConnection(fd);
        return -1;
    }
    io_watcher_max_size = io_watcher_max_size_;
    result = Realloc((void**)&write_watcher_, &io_watcher_max_size, fd,
            sizeof(struct ev_io*));
    if(0 > result){
        perror("realloc:");
        CloseConnection(fd);
        return -1;
    }

    io_watcher_max_size_ = io_watcher_max_size;

    struct ev_io* read_watcher = (struct ev_io*)malloc(sizeof(struct ev_io));
    if(NULL == read_watcher){
        CloseConnection(fd);
        return -1;
    }

    struct ev_io* write_watcher = (struct ev_io*)malloc(sizeof(struct ev_io));
    if(NULL == write_watcher){
        ::free(read_watcher);
        CloseConnection(fd);
        return -1;
    }

    //printf("new connection:%d\n", fd);
    read_watcher->data = this;
    write_watcher->data = this;
    read_watcher->fd = fd;
    write_watcher->fd = fd;
    read_watcher_[fd] = read_watcher;
    write_watcher_[fd] = write_watcher;
    ev_io_init(read_watcher, OnRead, fd, EV_READ);
    ev_io_start(loop_, read_watcher);
    return 0;
}

google::protobuf::Service* Channel::GetServiceByName(const std::string& name)
{
    for(uint32_t i = 0; i < service_current_size_; ++i){
        std::string _name = service_[i]->GetDescriptor()->full_name();
        if(NULL != service_[i] && name == service_[i]->GetDescriptor()->full_name()){
            return service_[i];
        }
    }
    return NULL;
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
int32_t Channel::Realloc(void** ptr, uint32_t* origin_length, uint32_t expect_length, uint32_t type_size)
{
    if(NULL == ptr || NULL == origin_length){
        ::puts("libmaid: ptr and origin_size can not be NULL in Realloc");
        return -1;
    }

    uint32_t new_length = *origin_length;
    while(new_length <= expect_length){
        new_length += 1;
        new_length <<= 1;
        void* new_ptr = ::realloc(*ptr, new_length * type_size);
        if(NULL == new_ptr){
            ::perror("Realloc:");
            return -1;
        }
        *ptr = new_ptr;
    }
    ::memset(*ptr + (*origin_length * type_size), 0, (new_length - *origin_length) * type_size);
    *origin_length = new_length;
    return 0;
}
