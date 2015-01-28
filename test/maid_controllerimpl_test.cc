#include "gtest/gtest.h"
#include "controllerimpl.h"
#include "maid/controller.pb.h"

TEST(ControllerImpl, proto) {
    maid::ControllerImpl controllerimpl;

    ASSERT_STREQ("", controllerimpl.proto().method_name().c_str());
    ASSERT_EQ(0u, controllerimpl.proto().transmit_id());
    ASSERT_FALSE(controllerimpl.proto().stub());
    ASSERT_FALSE(controllerimpl.proto().is_canceled());
    ASSERT_FALSE(controllerimpl.proto().failed());
    ASSERT_STREQ("", controllerimpl.proto().error_text().c_str());
    ASSERT_FALSE(controllerimpl.proto().notify());
    ASSERT_STREQ("", controllerimpl.proto().message().c_str());
    ASSERT_STREQ("", controllerimpl.proto().full_service_name().c_str());
}

TEST(ControllerImpl, ConnectionIdGetSet) {

    maid::ControllerImpl controllerimpl;
    ASSERT_EQ(0, controllerimpl.connection_id());

    controllerimpl.set_connection_id(1);
    ASSERT_EQ(1, controllerimpl.connection_id());

    controllerimpl.set_connection_id(-1);
    ASSERT_EQ(-1, controllerimpl.connection_id());
}

TEST(ControllerImpl, Reset)
{
}

TEST(ControllerImpl, FailedGetSet)
{
    maid::ControllerImpl controllerimpl;
    ASSERT_FALSE(controllerimpl.Failed());
    ASSERT_FALSE(controllerimpl.proto().failed());
    ASSERT_STREQ("", controllerimpl.ErrorText().c_str());

    controllerimpl.SetFailed("whatever");
    ASSERT_TRUE(controllerimpl.Failed());
    ASSERT_STREQ("whatever", controllerimpl.ErrorText().c_str());
}

TEST(ControllerImpl, StartCancel)
{
}

TEST(ControllerImpl, IsCanceled)
{
    maid::ControllerImpl controllerimpl;

    ASSERT_FALSE(controllerimpl.IsCanceled());
}
