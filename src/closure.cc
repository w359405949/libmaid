#include <stdio.h>
#include <glog/logging.h>
#include "maid/controller.pb.h"
#include "maid/connection.pb.h"

#include "closure.h"
#include "channel.h"
#include "controller.h"
#include "wire_format.h"
#include "helper.h"
#include "uv_hook.h"


namespace maid {

std::map<GCClosure*, GCClosure*> GCClosure::closures_;

Closure::Closure()
{
}

Closure::~Closure()
{
}

void Closure::Run()
{
}

GCClosure::GCClosure(google::protobuf::RpcController* controller,
        google::protobuf::Message* request,
        google::protobuf::Message* response)
    :controller_(controller),
    request_(request),
    response_(response)
{
    gc_.data = this;
}

void GCClosure::Run()
{
    CHECK(closures_.find(this) == closures_.end());
    closures_[this] = this;
    uv_idle_init(maid_default_loop(), &gc_);
    uv_idle_start(&gc_, OnGC);
}

void GCClosure::OnGC(uv_idle_t* handle)
{
    uv_idle_stop(handle);
    GCClosure* self = (GCClosure*)handle->data;
    if (closures_.find(self) == closures_.end()) {
        LOG(WARNING)<<"what happend";
        return;
    }

    closures_.erase(self);
    //delete self;
}

GCClosure::~GCClosure()
{
    delete controller_;
    delete request_;
    delete response_;
}

TcpClosure::TcpClosure(TcpChannel* channel, Controller* controller, google::protobuf::Message* request, google::protobuf::Message* response)
    :channel_(channel),
    send_buffer_(NULL),
    controller_(controller),
    request_(request),
    response_(response)
{
    req_.data = this;
}

void TcpClosure::Run()
{
    CHECK(send_buffer_ == NULL) << "TcpClosure::Run() call twice";
    channel_->RemoveController(controller_);

    if (controller_->IsCanceled() || helper::ProtobufHelper::notify(controller_->proto())) {
        gc_.data = this;
        uv_idle_init(maid_default_loop(), &gc_);
        uv_idle_start(&gc_, OnGc);
        return;
    }


    proto::ControllerProto* controller_proto = controller_->mutable_proto();
    controller_proto->ClearExtension(proto::connection);
    controller_proto->set_stub(false);
    if (!controller_->Failed()) {
        response_->SerializeToString(controller_proto->mutable_message());
    }
    send_buffer_ = WireFormat::Serializer(controller_->proto());

    uv_buf_t uv_buf;
    uv_buf.base = (char*)send_buffer_->data();
    uv_buf.len = send_buffer_->size();
    int error = uv_write(&req_, channel_->stream(), &uv_buf, 1, AfterSendResponse);
    if (error) {
        gc_.data = this;
        uv_idle_init(maid_default_loop(), &gc_);
        uv_idle_start(&gc_, OnGc);
        DLOG(ERROR) << uv_strerror(error);
        return;
    }
}

TcpClosure::~TcpClosure()
{
    delete send_buffer_;
    delete controller_;
    delete request_;
    delete response_;
}

void TcpClosure::OnGc(uv_idle_t* idle)
{
    uv_idle_stop(idle);
    TcpClosure* self = (TcpClosure*)idle->data;

    delete self;
}

void TcpClosure::AfterSendResponse(uv_write_t* handle, int32_t status)
{
    TcpClosure* self = (TcpClosure*)handle->data;
    delete self;
}

}
