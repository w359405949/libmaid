#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <uv.h>

#include <glog/logging.h>

#include "maid/controller.h"
#include "maid/controller.pb.h"
#include "maid/options.pb.h"
#include "maid/middleware.pb.h"
#include "maid/connection.pb.h"
#include "middleware/log_middleware.h"
#include "channelimpl.h"
#include "remoteclosure.h"
#include "define.h"
#include "buffer.h"
#include "context.h"
#include "mock.h"

using maid::ChannelImpl;

ChannelImpl::ChannelImpl(uv_loop_t* loop)
    :default_stream_(NULL),
     controller_max_size_(CONTROLLERMETA_MAX_SIZE),
     transmit_id_max_(0)
{
    signal(SIGPIPE, SIG_IGN);
    loop_ = loop;
    if (NULL == loop_) {
        loop_ = uv_default_loop();
    }
}

ChannelImpl::~ChannelImpl()
{
    for (std::map< std::string, google::protobuf::Service*>::iterator it = service_.begin(); it != service_.end(); it++) {
        delete it->second;
    }
    service_.clear();

    for (std::vector<proto::Middleware*>::iterator it = middleware_.begin(); it != middleware_.end(); it++) {
        delete *it;
    }
    middleware_.clear();

    for (std::map<int64_t, uv_stream_t*>::iterator it = connected_handle_.begin(); it != connected_handle_.end(); it++) {
        RemoveConnection(it->second);
    }
    connected_handle_.clear();

    for (std::map<int64_t, uv_stream_t*>::iterator it = listen_handle_.begin(); it != listen_handle_.end(); it++) {
        uv_close((uv_handle_t*)it->second, OnCloseListen);
    }
    listen_handle_.clear();

    loop_ = NULL;
}

void ChannelImpl::AppendService(google::protobuf::Service* service)
{
    DLOG_IF(FATAL, service_.find(service->GetDescriptor()->full_name()) != service_.end()) << " same name service regist twice";
    service_[service->GetDescriptor()->full_name()] = service;
}

void ChannelImpl::AppendMiddleware(proto::Middleware* middleware)
{
    middleware_.push_back(middleware);
}

void ChannelImpl::CallMethod(const google::protobuf::MethodDescriptor* method,
        google::protobuf::RpcController* rpc_controller,
        const google::protobuf::Message* request,
        google::protobuf::Message* response,
        google::protobuf::Closure* done)
{
    DCHECK(NULL != dynamic_cast<Controller*>(rpc_controller));
    DCHECK(NULL != request);

    Controller* controller = (Controller*)rpc_controller;
    controller->mutable_proto()->set_full_service_name(method->service()->full_name());
    controller->mutable_proto()->set_method_name(method->name());

    bool notify = false;
    const proto::MaidMethodOptions& method_options = method->options().GetExtension(proto::method_options);
    if (method_options.has_notify()) {
        notify = method_options.notify();
    } else {
        const proto::MaidServiceOptions& service_options = method->service()->options().GetExtension(proto::service_options);
        notify = service_options.notify();
    }

    if (!notify) {
        uint32_t transmit_id = ++transmit_id_max_; // TODO: check if used
        //DLOG_IF(FATAL, async_result_.find(transmit_id) != async_result_.end()) << "transmit id used";
        controller->mutable_proto()->set_transmit_id(transmit_id);
        async_result_[transmit_id].controller = controller;
        async_result_[transmit_id].response = response;
        async_result_[transmit_id].done = done;
        transactions_[controller->connection_id()].insert(transmit_id);
    } else {
        DLOG_IF(FATAL, NULL != done) << method->full_name() << " Closure should be NULL, while MaidMethodOptions.notify=true or MaidServiceOptions.notify=true";
    }

    try {
        request->SerializeToString(controller->mutable_proto()->mutable_message());
    } catch (std::bad_alloc) {
        controller->SetFailed("busy");
        HandleResponse(controller->mutable_proto(), controller->connection_id());
        return;
    }

    SendRequest(method, controller);
}


void ChannelImpl::SendRequest(const google::protobuf::MethodDescriptor* method, Controller* controller)
{
    controller->mutable_proto()->set_stub(true);

    bool notify = false;
    const proto::MaidMethodOptions& method_options = method->options().GetExtension(proto::method_options);
    if (method_options.has_notify()) {
        notify = method_options.notify();
    } else {
        const proto::MaidServiceOptions& service_options = method->service()->options().GetExtension(proto::service_options);
        notify = service_options.notify();
    }

    /*
     * middleware
     */
    std::vector<proto::Middleware*>::iterator it = middleware_.begin();
    for (; it != middleware_.end(); it++) {
        (*it)->ProcessRequest(MockController::default_instance(), controller->mutable_proto(), controller->mutable_proto(), MockClosure::default_instance());
    }

    std::string* send_buffer = NULL;
    try {
        send_buffer = new std::string();
        int32_t controller_size_nl = htonl(controller->proto().ByteSize());
        send_buffer->append((const char*)&controller_size_nl, sizeof(controller_size_nl));
        controller->proto().AppendToString(send_buffer);
    } catch (std::bad_alloc) {
        delete send_buffer;
        controller->SetFailed("busy");
        HandleResponse(controller->mutable_proto(), controller->connection_id());
        return;
    }

    uv_stream_t* stream = connected_handle(controller);
    if (NULL == stream) {
        delete send_buffer;
        controller->SetFailed("not connected");
        HandleResponse(controller->mutable_proto(), controller->connection_id());
        return;
    }

    controller->set_connection_id((int64_t)stream);

    uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
    if (NULL == req) {
        delete send_buffer;
        controller->SetFailed("busy");
        HandleResponse(controller->mutable_proto(), controller->connection_id());
        return;
    }
    req->data = notify ? NULL : controller;

    uv_buf_t uv_buf;
    uv_buf.base = (char*)send_buffer->data();
    uv_buf.len = send_buffer->size();
    int error = uv_write(req, stream, &uv_buf, 1, AfterSendRequest);
    if (error) {
        free(req);
        delete send_buffer;
        controller->SetFailed(uv_strerror(error));
        HandleResponse(controller->mutable_proto(), controller->connection_id());
        return;
    }

    sending_buffer_[req] = send_buffer;
}

void ChannelImpl::AfterSendRequest(uv_write_t* req, int32_t status)
{
    ChannelImpl* self = (ChannelImpl*)req->handle->data;
    Controller* controller = (Controller*)req->data;
    delete self->sending_buffer_[req];
    self->sending_buffer_.erase(req);
    delete req;

    if (status && NULL != controller) {
        controller->SetFailed(uv_strerror(status));
        self->HandleResponse(controller->mutable_proto(), controller->connection_id());
    }
}

void ChannelImpl::AfterSendResponse(uv_write_t* req, int32_t status)
{
    ChannelImpl* self = (ChannelImpl*)req->handle->data;

    delete self->sending_buffer_[req];
    self->sending_buffer_.erase(req);
    delete req;
}

void ChannelImpl::SendResponse(Controller* controller, const google::protobuf::Message* response)
{
    controller->mutable_proto()->set_stub(false);
    controller->mutable_proto()->clear_message();
    try {
        if (NULL != response) {
            response->SerializeToString(controller->mutable_proto()->mutable_message());
        }
    } catch (std::bad_alloc) {
        LOG(ERROR) << " no more memory";
        return;
    }

    /*
     * middleware
     */
    std::vector<proto::Middleware*>::iterator it = middleware_.begin();
    for (; it != middleware_.end(); it++) {
        (*it)->ProcessResponseStub(MockController::default_instance(), controller->mutable_proto(), controller->mutable_proto(), MockClosure::default_instance());
    }

    std::string* buffer = NULL;
    try {
        buffer = new std::string();
        int32_t controller_nl = htonl(controller->proto().ByteSize());
        buffer->append((char*)&controller_nl, sizeof(controller_nl));
        controller->proto().AppendToString(buffer);
    } catch (std::bad_alloc) {
        delete buffer;
        LOG(ERROR) << " no more memory";
        return;
    }


    uv_stream_t* stream = connected_handle(controller);
    if (NULL == stream) {
        delete buffer;
        LOG(ERROR) << " connection: " << controller->connection_id() << " not connected";
        return;
    }

    uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
    if (NULL == req) {
        delete buffer;
        LOG(ERROR) << " no more memory";
        return;
    }

    uv_buf_t uv_buf;
    uv_buf.base = (char*)buffer->data();
    uv_buf.len = buffer->size();
    int error = uv_write(req, stream, &uv_buf, 1, AfterSendResponse);
    if (error) {
        free(req);
        delete buffer;
        DLOG(FATAL) << uv_strerror(error);
        return;
    }

    sending_buffer_[req] = buffer;
}

void ChannelImpl::OnRead(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf)
{
    DLOG(INFO) << "connection: " << (int64_t)stream << " nread: " << nread;
    ChannelImpl * self = (ChannelImpl*)stream->data;

    if (nread < 0) {
        self->RemoveConnection(stream);
        return;
    }

    /*
     * TODO: buf->base alloced by libuv (win).
     */
    self->buffer_[(int64_t)stream].end += nread;

    uv_read_stop(stream);
    //uv_timer_t& timer = self->timer_handle_[(int64_t)stream];
    //uv_timer_start(&timer, OnTimer, 0, 0);
    //uv_check_t& check = self->check_handle_[(int64_t)stream];
    //uv_check_start(&check, OnCheck);
    uv_idle_t& idle = self->idle_handle_[(int64_t)stream];
    uv_idle_start(&idle, OnIdle);
}


void ChannelImpl::OnIdle(uv_idle_t* idle)
{
    uv_stream_t* stream = (uv_stream_t*)idle->data;
    ChannelImpl* self = (ChannelImpl*)stream->data;
    int32_t result = self->Handle((int64_t)stream);

    // scheduler
    switch (result) {
        case ERROR_OUT_OF_SIZE:
            break;
        case ERROR_LACK_DATA:
            uv_idle_stop(idle);
            uv_read_start(stream, OnAlloc, OnRead);
            return;
            break;
        case ERROR_BUSY:
            break;
        case ERROR_PARSE_FAILED:
            break;
        case ERROR_OTHER:
            break;
        default:
            break;
    }
}



void ChannelImpl::OnCheck(uv_check_t* check)
{
    uv_stream_t* stream = (uv_stream_t*)check->data;
    ChannelImpl* self = (ChannelImpl*)stream->data;
    int32_t result = self->Handle((int64_t)stream);

    // scheduler
    switch (result) {
        case ERROR_OUT_OF_SIZE:
            break;
        case ERROR_LACK_DATA:
            uv_check_stop(check);
            uv_read_start(stream, OnAlloc, OnRead);
            break;
        case ERROR_BUSY:
            break;
        case ERROR_PARSE_FAILED:
            break;
        case ERROR_OTHER:
            break;
        default:
            break;
    }

    uv_check_stop(check);
    uv_check_start(check, OnCheck);
}

void ChannelImpl::OnTimer(uv_timer_t* timer)
{
    uv_stream_t* stream = (uv_stream_t*)timer->data;
    ChannelImpl* self = (ChannelImpl*)stream->data;
    int32_t result = self->Handle((int64_t)stream);

    // scheduler
    switch (result) {
        case ERROR_OUT_OF_SIZE:
            break;
        case ERROR_LACK_DATA:
            uv_timer_stop(timer);
            uv_read_start(stream, OnAlloc, OnRead);
            break;
        case ERROR_BUSY:
            break;
        case ERROR_PARSE_FAILED:
            break;
        case ERROR_OTHER:
            break;
        default:
            break;
    }

    uv_timer_stop(timer);
    uv_timer_start(timer, OnTimer, 0, 0);
}

int32_t ChannelImpl::Handle(int64_t connection_id)
{
    Buffer& buffer = buffer_[connection_id];

    int32_t controller_size_nl = 0;
    if (buffer.start + sizeof(controller_size_nl) > buffer.end) {
        return ERROR_LACK_DATA;
    }

    memcpy(&controller_size_nl, buffer.start, sizeof(controller_size_nl));
    int32_t controller_size = ntohl(controller_size_nl);

    if (controller_size < 0 || controller_size > controller_max_size_) {
        buffer.start = buffer.end;
        DLOG(FATAL) << " connection: " << connection_id << " controller_size: " << controller_size << " out of max size: " << controller_max_size_;
        return ERROR_OUT_OF_SIZE;
    }

    if (buffer.start + sizeof(controller_size_nl) + controller_size > buffer.end) {
        return ERROR_LACK_DATA;
    }

    proto::ControllerProto proto;
    try {
        if (!proto.ParseFromArray(buffer.start + sizeof(controller_size_nl), controller_size)) {
            DLOG(FATAL) << " connection: " << connection_id << " parse ControllerProto failed";
            buffer.start += sizeof(controller_size_nl) + controller_size;
            return ERROR_PARSE_FAILED;
        }
    } catch (std::bad_alloc) {
        return ERROR_BUSY;
    }
    buffer.start += sizeof(controller_size_nl) + controller_size;
    CHECK(buffer.start <= buffer.end);

    if (proto.stub()) {
        return HandleRequest(&proto, connection_id);
    } else {
        return HandleResponse(&proto, connection_id);
    }

    return 0;
}

int32_t ChannelImpl::HandleRequest(proto::ControllerProto* proto, int64_t connection_id)
{
    /*
     * middleware
     */
    std::vector<proto::Middleware*>::iterator middleware_it = middleware_.begin();
    for (; middleware_it != middleware_.end(); middleware_it++) {
        (*middleware_it)->ProcessRequestStub(MockController::default_instance(), proto, proto, MockClosure::default_instance());
    }

    std::map<std::string, google::protobuf::Service*>::iterator service_it;
    service_it = service_.find(proto->full_service_name());
    if (service_.end() == service_it) {
        DLOG(WARNING) << " service: " << proto->full_service_name().c_str() << " not exist";
        return ERROR_OTHER;
    }

    google::protobuf::Service* service = service_it->second;
    const google::protobuf::MethodDescriptor* method = NULL;
    method = service->GetDescriptor()->FindMethodByName(proto->method_name());
    if (NULL == method) {
        DLOG(WARNING) << " service: " << proto->full_service_name().c_str() << " method: " << proto->method_name() << " not exist";
        return ERROR_OTHER;
    }

    bool notify = false;
    const proto::MaidMethodOptions& method_options = method->options().GetExtension(proto::method_options);
    if (method_options.has_notify()) {
        notify = method_options.notify();
    } else {
        const proto::MaidServiceOptions& service_options = method->service()->options().GetExtension(proto::service_options);
        notify = service_options.notify();
    }

    google::protobuf::RpcController* controller = MockController::default_instance();
    google::protobuf::Message* request = NULL;
    google::protobuf::Message* response = NULL;
    google::protobuf::Closure* done = MockClosure::default_instance();

    try {
        request = service->GetRequestPrototype(method).New();
        if (!request->ParseFromString(proto->message())) {
            delete request;
            delete done;
            return ERROR_PARSE_FAILED;
        }

        if (!notify) {
            RemoteClosure* closure = NewRemoteClosure();
            done = closure;

            Controller* maid_controller = new Controller();
            controller = maid_controller;
            proto->clear_message();
            maid_controller->mutable_proto()->CopyFrom(*proto);
            maid_controller->set_connection_id(connection_id);

            closure->set_controller(maid_controller);

            closure->set_request(request);

            response = service->GetResponsePrototype(method).New();
            closure->set_response(response);
        }
    } catch (std::bad_alloc) {
        if (controller != MockController::default_instance())
        {
            delete done;
        }
        delete request;
        delete response;
        if (done!= MockClosure::default_instance())
        {
            delete done;
        }
        return ERROR_BUSY;
    }

    CHECK(connection_id == ((Controller*)controller)->connection_id());
    // call
    service->CallMethod(method, controller, request, response, done);

    if (notify) {
        delete request;
    }
    return 0;
}

int32_t ChannelImpl::HandleResponse(proto::ControllerProto* proto, int64_t connection_id)
{
    /*
     * middleware
     */
    std::vector<proto::Middleware*>::iterator middleware_it = middleware_.begin();
    for (; middleware_it != middleware_.end(); middleware_it++) {
        (*middleware_it)->ProcessResponse(MockController::default_instance(), proto, proto, MockClosure::default_instance());
    }

    std::map<int64_t, Context>::iterator it = async_result_.find(proto->transmit_id());
    Context& context = it->second;
    if (it == async_result_.end() || context.controller->connection_id() != connection_id) {
        return 0;
    }
    it->second.response->ParseFromString(proto->message());
    proto->clear_message();
    it->second.controller->mutable_proto()->CopyFrom(*proto);
    it->second.done->Run();

    async_result_.erase(it->first);
    transactions_[connection_id].erase(it->first);
    if (transactions_[connection_id].size() == 0) {
        transactions_.erase(connection_id);
    }

    return 0;
}

maid::RemoteClosure* ChannelImpl::NewRemoteClosure()
{
    RemoteClosure* done = NULL;
    if (!remote_closure_pool_.empty()) {
        done = remote_closure_pool_.top();
        remote_closure_pool_.pop();
    } else {
        done = new RemoteClosure(this);
    }
    return done;
}

void ChannelImpl::DeleteRemoteClosure(maid::RemoteClosure* done)
{
    if (done != NULL) {
        remote_closure_pool_.push(done);
    }
}

void ChannelImpl::OnConnect(uv_connect_t* req, int32_t status)
{
    ChannelImpl* self = (ChannelImpl*)req->data;
    if (!status) {
        uv_tcp_nodelay((uv_tcp_t*)req->handle, 1);
        self->AddConnection(req->handle);
    } else if (req->handle == self->default_stream_) {
        self->default_stream_ = NULL;
    }
    free(req);
}

int64_t ChannelImpl::Connect(const char* host, int32_t port, bool as_default)
{
    uv_tcp_t* handle = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    uv_connect_t* req = (uv_connect_t*)malloc(sizeof(uv_connect_t));
    if (NULL == handle || NULL == req) {
        free(handle);
        free(req);
        return ERROR_BUSY;
    }

    uv_tcp_init(loop_, handle);

    struct sockaddr_in address;
    uv_ip4_addr(host, port, &address);
    req->data = this;

    int result = uv_tcp_connect(req, handle, (struct sockaddr*)&address, OnConnect);
    if (result) {
        uv_close((uv_handle_t*)handle, OnCloseConnection);
        free(req);
        DLOG(WARNING) << uv_strerror(result);
        return ERROR_OTHER;
    }

    if (as_default) {
        default_stream_ = (uv_stream_t*)handle;
    }

    return (int64_t)(handle);
}

void ChannelImpl::OnAccept(uv_stream_t* handle, int32_t status)
{
    if (status) {
        return;
    }

    uv_tcp_t* peer_handle = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    if (NULL == peer_handle) {
        DLOG(WARNING) << " no more memory, denied";
        return;
    }

    uv_tcp_init(handle->loop, peer_handle);
    int result = uv_accept(handle, (uv_stream_t*)peer_handle);
    if (result) {
        DLOG(WARNING) << uv_strerror(result);
        uv_close((uv_handle_t*)peer_handle, OnCloseConnection);
        return;
    }

    uv_tcp_nodelay(peer_handle, 1);
    ChannelImpl* self = (ChannelImpl*)handle->data;
    self->AddConnection((uv_stream_t*)peer_handle);
}

void ChannelImpl::OnCloseListen(uv_handle_t* handle)
{
    ChannelImpl* self = (ChannelImpl*)handle->data;
    self->listen_handle_.erase((int64_t)handle);
    free(handle);
}

int64_t ChannelImpl::Listen(const char* host, int32_t port, int32_t backlog)
{
    uv_tcp_t* handle = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    if (NULL == handle) {
        return ERROR_BUSY;
    }

    uv_tcp_init(loop_, handle);
    struct sockaddr_in address;
    int32_t result = 0;
    result = uv_ip4_addr(host, port, &address);
    if (result) {
        uv_close((uv_handle_t*)handle, OnCloseListen);
        DLOG(WARNING) << uv_strerror(result);
        return ERROR_OTHER;
    }

    int32_t flags = 0;
    result = uv_tcp_bind(handle, (struct sockaddr*)&address, flags);
    if (result) {
        uv_close((uv_handle_t*)handle, OnCloseListen);
        DLOG(WARNING) << uv_strerror(result);
        return ERROR_OTHER;
    }
    result = uv_listen((uv_stream_t*)handle, backlog, OnAccept);
    if (result) {
        uv_close((uv_handle_t*)handle, OnCloseListen);
        DLOG(WARNING) << uv_strerror(result);
        return ERROR_OTHER;
    }

    handle->data = this;
    listen_handle_[(int64_t)handle] = (uv_stream_t*)handle;
    return (int64_t)handle;
}

void ChannelImpl::OnCloseConnection(uv_handle_t* handle)
{
    ChannelImpl* self = (ChannelImpl*)handle->data;
    self->connected_handle_.erase((int64_t)handle);
    free(handle);
}

void ChannelImpl::RemoveConnection(uv_stream_t* stream)
{
    uv_read_stop(stream);
    buffer_.erase((int64_t)(stream));

    uv_idle_stop(&idle_handle_[(int64_t)stream]);
    idle_handle_.erase((int64_t)stream);

    uv_check_stop(&check_handle_[(int64_t)stream]);
    check_handle_.erase((int64_t)stream);

    uv_timer_stop(&timer_handle_[(int64_t)stream]);
    timer_handle_.erase((int64_t)stream);

    if (default_stream_ == stream) {
        default_stream_ = NULL;
    }

    stream->data = this;
    uv_close((uv_handle_t*)stream, OnCloseConnection);

    /*
     * middleware
     */
    proto::ConnectionProto connection;
    connection.set_id((int64_t)stream);
    std::vector<proto::Middleware*>::iterator middleware_it = middleware_.begin();
    for (; middleware_it != middleware_.end(); middleware_it++) {
        (*middleware_it)->Disconnected(MockController::default_instance(), &connection, &connection, MockClosure::default_instance());
    }
}

void ChannelImpl::AddConnection(uv_stream_t* stream)
{
    stream->data = this;
    connected_handle_[(int64_t)stream] = stream;
    uv_read_start(stream, OnAlloc, OnRead);

    uv_check_t& check = check_handle_[(int64_t)stream];
    check.data = stream;
    uv_check_init(loop_, &check);

    uv_idle_t& idle = idle_handle_[(int64_t)stream];
    idle.data = stream;
    uv_idle_init(loop_, &idle);

    uv_timer_t& timer = timer_handle_[(int64_t)stream];
    timer.data = stream;
    uv_timer_init(loop_, &timer);

    /*
     * middleware
     */
    proto::ConnectionProto connection;
    connection.set_id((int64_t)stream);
    std::vector<proto::Middleware*>::iterator it = middleware_.begin();
    for (; it != middleware_.end(); it++) {
        (*it)->Connected(MockController::default_instance(), &connection, &connection, MockClosure::default_instance());
    }
}

void ChannelImpl::OnAlloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
    ChannelImpl* self = (ChannelImpl*)handle->data;
    Buffer& buffer = self->buffer_[(int64_t)(handle)];

    buf->len = buffer.Expend(suggested_size);
    buf->base = (char*)buffer.end;
}

uv_stream_t* ChannelImpl::connected_handle(Controller* controller)
{
    const proto::ConnectionProto& connection = controller->proto().GetExtension(proto::connection);
    if (!connection.has_id()) {
        return default_stream_;
    }

    uv_stream_t* handle = NULL;
    std::map<int64_t, uv_stream_t*>::const_iterator it = connected_handle_.find(connection.id());
    if (connected_handle_.end() != it) {
        handle = it->second;
    }
    return handle;
}

void ChannelImpl::set_default_connection_id(int64_t connection_id)
{
    std::map<int64_t, uv_stream_t*>::const_iterator it = connected_handle_.find(connection_id);
    if (connected_handle_.end() != it) {
        default_stream_ = it->second;
    }
}

int64_t ChannelImpl::default_connection_id()
{
    return (int64_t)default_stream_;
}
