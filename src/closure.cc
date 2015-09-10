#include <google/protobuf/empty.pb.h>
#include <google/protobuf/stubs/logging.h>
#include "maid/controller.pb.h"
#include "maid/connection.pb.h"

#include "closure.h"
#include "channel.h"
#include "controller.h"
#include "wire_format.h"


namespace maid {

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
}

void GCClosure::Run()
{
    GOOGLE_CHECK(controller_ != NULL);
    delete controller_;
    delete request_;
    delete response_;
    delete this;
}

GCClosure::~GCClosure()
{
    controller_ = NULL;
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
    channel_->RemoveController(controller_);

    if (controller_->IsCanceled() || response_->GetDescriptor() == google::protobuf::Empty::descriptor()) {
        delete this;
        return;
    }


    proto::ControllerProto* controller_proto = controller_->mutable_proto();
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
        GOOGLE_DLOG(ERROR) << uv_strerror(error);
        delete this;
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

void TcpClosure::AfterSendResponse(uv_write_t* handle, int32_t status)
{
    TcpClosure* self = (TcpClosure*)handle->data;
    delete self;
}

}
