#include "gtest/gtest.h"
#include "controllerimpl.h"

TEST(ControllerImpl, MetaData) {
    maid::ControllerImpl controllerimpl;

    ASSERT_STREQ("", controllerimpl.meta_data().method_name().c_str());
    ASSERT_EQ(0u, controllerimpl.meta_data().transmit_id());
    ASSERT_FALSE(controllerimpl.meta_data().stub());
    ASSERT_FALSE(controllerimpl.meta_data().is_canceled());
    ASSERT_FALSE(controllerimpl.meta_data().failed());
    ASSERT_STREQ("", controllerimpl.meta_data().error_text().c_str());
    ASSERT_FALSE(controllerimpl.meta_data().notify());
    ASSERT_STREQ("", controllerimpl.meta_data().message().c_str());
    ASSERT_STREQ("", controllerimpl.meta_data().full_service_name().c_str());
}

TEST(ControllerImpl, FdGetSet) {

    maid::ControllerImpl controllerimpl;
    ASSERT_EQ(0, controllerimpl.fd());

    controllerimpl.set_fd(1);
    ASSERT_EQ(1, controllerimpl.fd());

    controllerimpl.set_fd(-1);
    ASSERT_EQ(-1, controllerimpl.fd());
}

TEST(ControllerImpl, Reset)
{
}

TEST(ControllerImpl, FailedGetSet)
{
    maid::ControllerImpl controllerimpl;
    ASSERT_FALSE(controllerimpl.Failed());
    ASSERT_FALSE(controllerimpl.meta_data().failed());
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
