#include "gtest/gtest.h"
#include "controller.h"
#include "maid/controller.pb.h"

TEST(Controller, proto)
{
    maid::Controller controller;

    ASSERT_TRUE(&maid::proto::ControllerProto::default_instance() == &controller.proto());
    ASSERT_TRUE(NULL == controller.release_proto());

    maid::proto::ControllerProto* proto = controller.mutable_proto();
    ASSERT_TRUE(NULL != proto);
    ASSERT_TRUE(controller.mutable_proto() != &maid::proto::ControllerProto::default_instance());

    maid::proto::ControllerProto* release_proto = controller.release_proto();
    ASSERT_EQ(release_proto, proto);
    ASSERT_TRUE(&maid::proto::ControllerProto::default_instance() == &controller.proto());
    ASSERT_TRUE(NULL == controller.release_proto());
}

TEST(Controller, FailedGetSet)
{
    maid::Controller controller;

    ASSERT_FALSE(controller.Failed());
    ASSERT_FALSE(controller.proto().failed());
    ASSERT_TRUE(controller.ErrorText().empty());

    std::string str = "whatever";
    controller.SetFailed(str);
    ASSERT_TRUE(controller.Failed());
    ASSERT_EQ(str, controller.ErrorText());
    ASSERT_TRUE(&maid::proto::ControllerProto::default_instance() != &controller.proto());
}

TEST(Controller, Cancel)
{
    maid::Controller controller;

    ASSERT_FALSE(controller.IsCanceled());

    controller.StartCancel();
    ASSERT_TRUE(controller.IsCanceled());
    ASSERT_TRUE(&maid::proto::ControllerProto::default_instance() != &controller.proto());
}
