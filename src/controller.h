#pragma once

#include <google/protobuf/service.h>
#include <google/protobuf/stubs/common.h>

#include "maid/controller.pb.h"

namespace maid
{

class Controller : public google::protobuf::RpcController
{
public:
    Controller()
        :proto_(NULL),
        cancel_callback_(NULL)
    {
    }


    virtual ~Controller()
    {
        delete cancel_callback_;
        delete proto_;
    }

    inline void Reset() override
    {
        if (NULL != proto_) {
            proto_->Clear();
        }
    }

    inline bool Failed() const override
    {
        return proto_ == NULL ? proto::ControllerProto::default_instance().failed() : proto_->failed();
    }

    inline std::string ErrorText() const override
    {
        return proto_ == NULL ? proto::ControllerProto::default_instance().error_text() : proto_->error_text();
    }

    inline void StartCancel() override
    {
        mutable_proto();

        GOOGLE_CHECK(!proto_->is_canceled()) << "controller start cancel called twice!";
        proto_->set_is_canceled(true);

        /*
        if (cancel_callback_ != NULL) {
            cancel_callback_->Run();
        }
        */
    }

    inline void SetFailed(const std::string& reason) override
    {
        mutable_proto();
        proto_->set_failed(true);
        proto_->set_error_text(reason);
    }

    inline bool IsCanceled() const override
    {
        return proto_ == NULL ? proto::ControllerProto::default_instance().is_canceled() : proto_->is_canceled();
    }

    void NotifyOnCancel(google::protobuf::Closure* callback) override
    {
        /*
        GOOGLE_CHECK(cancel_callback_ == NULL);
        cancel_callback_ = callback;
        */
    }

public:
    inline proto::ControllerProto* mutable_proto()
    {
        if (proto_ == NULL) {
            proto_ = new proto::ControllerProto();
        }
        return proto_;
    }

    inline const proto::ControllerProto& proto() const
    {
        return proto_ == NULL ? proto::ControllerProto::default_instance() : *proto_;
    }

    inline proto::ControllerProto* release_proto()
    {
        proto::ControllerProto* proto = proto_;
        proto_ = NULL;
        return proto;
    }

    inline void set_allocated_proto(proto::ControllerProto* proto)
    {
        if (proto == proto_) {
            return;
        }

        // special
        if (proto_ != nullptr) {
            proto->set_is_canceled(proto_->is_canceled());
        }

        delete proto_;
        proto_ = proto;
    }

public: // unit test only
    inline const google::protobuf::Closure* cancel_callback() const
    {
        return cancel_callback_;
    }

private:
    proto::ControllerProto* proto_;
    google::protobuf::Closure* cancel_callback_;

    GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(Controller);
};

} /* maid */
