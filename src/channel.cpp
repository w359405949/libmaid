#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <ev.h>
#include "channel.h"
#include "controller.h"
#include "closure.h"

using maid::channel::Channel;
using maid::controller::Controller;
using maid::proto::ControllerMeta;
using maid::channel::Context;
using maid::closure::RemoteClosure;

Channel::Channel(struct ev_loop* loop)
    :loop_(loop),
    read_watcher_(NULL),
    write_watcher_(NULL),
    io_watcher_max_size_(0),
    header_length_(8),
    buffer_(NULL),
    buffer_pending_index_(NULL),
    buffer_max_length_(NULL),
    buffer_list_max_size_(0),
    context_(NULL),
    context_list_max_size_(0)
{
    assert(("libmaid: loop can not be none", NULL != loop));
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
    assert(("service can not be NULL", NULL != service));
    int32_t result = Realloc((void**)&service_, &service_max_size_,
            service_current_size_, sizeof(google::protobuf::Service*));
    if(0 > result){
        return -1;
    }
    service_[service_current_size_++] = service;
    return 0;
}

void Channel::CallMethod(const google::protobuf::MethodDescriptor * method,
        google::protobuf::RpcController * controller,
        const google::protobuf::Message * request,
        google::protobuf::Message * response,
        google::protobuf::Closure * done)
{
    assert(("libmaid: controller must have type Controller", NULL != dynamic_cast<Controller *>(controller)));
    assert(("libmaid: request must not be NULL", NULL != request));
    assert(("libmaid: done can not be NULL", NULL != done));

    Context * context = (Context*)malloc(sizeof(Context));
    if(NULL == context){
        std::string error_text(strerror(errno));
        controller->SetFailed(error_text);
        done->Run();
        return;
    }
    context->method_ = method;
    context->controller_ = (Controller*)controller;
    context->request_ = (google::protobuf::Message*)request;
    context->response_ = response;
    context->done_ = done;

    /*
     *  assume the oldest id always handled first.
     *  so simple transmit_id search WOULD NOT ALWAYS BAD as O(n).
     *  TODO: more test and benchmark.
     */
    uint32_t transmit_id = context_list_max_size_;
    for(uint32_t i = 0; i < context_list_max_size_; ++i) {
        if(NULL != context_[i]){
            transmit_id = i;
            break;
        }
    }
    int32_t result = Realloc((void**)&context_, &context_list_max_size_, transmit_id, sizeof(struct Context));
    if(0 > result){
        std::string error_text(strerror(errno));
        controller->SetFailed(error_text);
        ::free(context);
        done->Run();
        return;
    }

    ControllerMeta& meta = ((Controller*)controller)->get_meta_data();

    meta.set_service_name(method->service()->full_name());
    meta.set_method_name(method->name());
    meta.set_transmit_id(transmit_id);
    meta.set_stub(true);

    context_[transmit_id] = context;

    if(meta.wide()){
        /*
         * BroadCast to all connected fd
         */
        Controller * con = (Controller*)controller;
        for(uint32_t fd = 0; fd < context_list_max_size_; ++fd) {

            if(0 > AppendContext(fd, context)){
                continue;
            }
            con->Ref();
        }
        if(con->get_ref() == 0){
            controller->SetFailed("no effective connection");
            done->Run();
            ::free(context);
        }
        return;
    }else{
        if(!AppendContext(meta.fd(), context)){
            controller->SetFailed("invalid fd");
            done->Run();
            return;
        }
    }
}

int32_t Channel::AppendContext(const int32_t fd, Context* context)
{
    if(!IsEffictive(fd)){
        return -1;
    }
    Context * head = context_[fd];
    if(NULL != head){
        for(; head->next_; head = head->next_);
        head->next_ = context;
    }else{
        head = context;
    }
    context_[fd] = head;
    return 0;
}

bool Channel::IsEffictive(int32_t fd) const
{
    return fd < io_watcher_max_size_ && 0 > fd && NULL != write_watcher_[fd] && NULL != read_watcher_[fd];
}

void Channel::OnWrite(EV_P_ ev_io * w, int revents)
{
    Channel* self = (Channel*)(w->data);
    Context* context = self->context_[w->fd];

    while(context){
        Controller * controller = context->controller_;
        ControllerMeta meta = controller->get_meta_data();

        /*
         * serializer can be cached if neccessary
         */
        std::string controller_buffer;
        uint32_t controller_nl = ::htonl(controller_buffer.length());
        meta.SerializeToString(&controller_buffer);

        std::string message_buffer;
        if(meta.stub()){
            context->request_->SerializeToString(&message_buffer);
        }else{
            context->response_->SerializeToString(&message_buffer);
        }

        uint32_t message_nl = ::htonl(message_buffer.length());
        int32_t result = 0;
        result += ::write(w->fd, &controller_nl, sizeof(controller_nl));
        if(0 > result){
            std::string error_text(strerror(errno));
            controller->SetFailed(error_text);
            context->done_->Run();
            self->CloseConnection(w->fd);
            return;
        }
        result += ::write(w->fd, &message_nl, sizeof(message_nl));
        if(0 > result){
            std::string error_text(strerror(errno));
            controller->SetFailed(error_text);
            context->done_->Run();
            self->CloseConnection(w->fd);
            return;
        }

        //if(!meta.SerializeToFileDescriptor(w->fd)){}
        result += ::write(w->fd, controller_buffer.c_str(),
                controller_buffer.length());
        if(0 > result){
            std::string error_text(strerror(errno));
            controller->SetFailed(error_text);
            context->done_->Run();
            self->CloseConnection(w->fd);
            return;
        }

        result += ::write(w->fd, message_buffer.c_str(),
                message_buffer.length());
        if(0 > result){
            std::string error_text(strerror(errno));
            controller->SetFailed(error_text);
            context->done_->Run();
            self->CloseConnection(w->fd);
            return;
        }
        assert(("libmaid: lose data", self->header_length_ + controller_buffer.length() + message_buffer.length() == result));

        if(!meta.stub()){
            Context* tmp = context;
            context = context->next_;
            ::free(tmp);
        }else{
            context = context->next_;
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
    result = Realloc((void**)&(self->buffer_[w->fd]),
            &(self->buffer_max_length_[w->fd]),
            self->buffer_pending_index_[w->fd], sizeof(int8_t));
    if(0 > result){
        self->CloseConnection(w->fd);
        perror("realloc:");
        return;
    }

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
        memcpy(buffer, &controller_length_nl, 4);
        memcpy(buffer, &message_length_nl, 4);
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
        int8_t* message_start = buffer + handled_start + header_length_ + controller_length;
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

    buffer_pending_index_[fd] = buffer_pending_index;
    buffer_max_length_[fd] = buffer_max_length;
    buffer_[fd] = buffer;
}

int32_t Channel::HandleResponse(int32_t fd, ControllerMeta& meta,
        const int8_t* message_start, int32_t message_length)
{
    if(meta.transmit_id() < connect_watcher_max_size_){
        /*
         * TODO: do something while context is lost, log?
         */
        return message_length;
    }
    Context* context = context_[meta.transmit_id()];
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

    ControllerMeta& stub_meta = controller->get_meta_data();
    if(stub_meta.wide()){
        controller->Unref();
    }
    if(!controller->Failed()){
        google::protobuf::Message * new_response = context->response_->New();
        if(NULL == new_response){
            return 0; //delay
        }
        if(!new_response->ParseFromArray(message_start, message_length)){
            return -1;//can not recovered
        }
        response->MergeFrom(*new_response);
        delete new_response;
    }

    if(!stub_meta.wide() || !controller->get_ref()){
        context->done_->Run();
        ::free(context);
        context_[stub_meta.transmit_id()] = NULL;
    }
    return message_length;
}

/*
 * TODO: wide == true
 */
int32_t Channel::HandleRequest(int32_t fd, ControllerMeta& stub_meta,
        const int8_t* message_start, int32_t message_length)
{
    Context* context = new Context();
    if(NULL == context){
        return 0; // delay
    }
    context->controller_ = new Controller();
    if(NULL == context->controller_){
        delete context;
        return 0; // delay
    }
    context->done_ = new RemoteClosure(loop_, this, context);
    if(NULL == context->done_){
        delete context->controller_;
        delete context;
        return 0;
    }

    ControllerMeta& meta = context->controller_->get_meta_data();
    meta.CopyFrom(stub_meta);
    meta.set_stub(false);
    google::protobuf::Service* service = GetServiceByName(meta.method_name());
    if(NULL == service){
        delete context->done_;
        delete context->controller_;
        delete context;
        return -1;
    }
    const google::protobuf::ServiceDescriptor* service_descriptor = service->GetDescriptor();
    context->method_ = service_descriptor->FindMethodByName(meta.method_name());
    context->request_ = service->GetRequestPrototype(context->method_).New();
    if(NULL == context->request_){
        delete context->done_;
        delete context->controller_;
        delete context->request_;
        delete context;
        return 0;
    }
    if(!context->request_->ParseFromArray(message_start, message_length)){
        delete context->done_;
        delete context->controller_;
        delete context->request_;
        delete context;
        return -1;
    }
    context->response_ = service->GetResponsePrototype(context->method_).New();
    if(NULL == context->request_){
        delete context->done_;
        delete context->controller_;
        delete context->request_;
        delete context;
        return 0; // delay
    }
    service->CallMethod(context->method_, context->controller_,
            context->request_, context->response_, context->done_);
    return message_length;
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

    result = Realloc((void**)&connect_watcher_, &connect_watcher_max_size_, fd,
            sizeof(struct ev_io*));
    if(0 > result){
        perror("realloc:");
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
            sizeof(struct ev_io));
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
        result = self->NewConnection(w->fd);
        if(0 > result){
            self->CloseConnection(w->fd);
        }
    }
}

void Channel::CloseConnection(int32_t fd)
{
    //
    ::close(fd);

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
            ev_io_stop(loop_, write_watcher);
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
        context_[fd] = NULL;
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
    result = Realloc((void**)&read_watcher_, &io_watcher_max_size,
            fd, sizeof(struct ev_io));
    if(0 > result){
        perror("realloc:");
        CloseConnection(fd);
        return -1;
    }
    io_watcher_max_size = io_watcher_max_size_;
    result = Realloc((void**)&write_watcher_, &io_watcher_max_size,
            fd, sizeof(struct ev_io));
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

    read_watcher->data = this;
    write_watcher->data = this;
    read_watcher->fd = fd;
    write_watcher->fd = fd;
    read_watcher_[fd] = read_watcher;
    write_watcher_[fd] = write_watcher;
    ev_io_init(read_watcher, OnRead, fd, EV_READ);
    ev_io_start(loop_, read_watcher);
    ev_io_init(write_watcher, OnWrite, fd, EV_WRITE);
    ev_io_start(loop_, write_watcher);
    return 0;
}

google::protobuf::Service* Channel::GetServiceByName(const std::string& name) const
{
    for(uint32_t i = 0; i < service_current_size_; ++i){
        if(NULL == service_[i] && name == service_[i]->GetDescriptor()->name()){
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
int32_t Channel::Realloc(void** ptr, uint32_t* origin_size,
        uint32_t expect_size, size_t type_size)
{
    if(NULL == ptr || NULL == origin_size){
        ::puts("libmaid: ptr and origin_size can not be NULL in Realloc");
        return -1;
    }

    uint32_t size = *origin_size;
    while(size  <= expect_size){
        size += 1;
        size <<= 1;
        void* new_ptr = ::realloc(*ptr, size);
        if(NULL == new_ptr){
            ::perror("Realloc:");
            return -1;
        }
        *ptr = new_ptr;
    }
    ::memset(*ptr + *origin_size, 0, size - *origin_size); //TODO:check
    *origin_size = size;
    return 0;
}
