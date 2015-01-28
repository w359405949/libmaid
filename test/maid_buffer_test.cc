#include "gtest/gtest.h"
#include "buffer.h"


TEST(Buffer, Constructor)
{
    maid::Buffer buffer;

    ASSERT_EQ(NULL, buffer.base_);
    ASSERT_EQ(NULL, buffer.start);
    ASSERT_EQ(NULL, buffer.end);
    ASSERT_EQ(0u, buffer.size_);
}

TEST(Buffer, Expend)
{
    maid::Buffer buffer;
    size_t expect_size = 10;

    buffer.Expend(expect_size);
    ASSERT_LE(expect_size, buffer.size_);
    ASSERT_EQ(buffer.base_, buffer.start);
    ASSERT_EQ(buffer.start, buffer.end);
}

TEST(Buffer, ExpendTwice)
{
    maid::Buffer buffer;
    size_t expect_size = 50;
    buffer.Expend(expect_size);
    size_t origin_size = buffer.size_;
    buffer.Expend(expect_size);

    ASSERT_LE(expect_size, buffer.size_);
    ASSERT_EQ(buffer.base_, buffer.start);
    ASSERT_EQ(buffer.start, buffer.end);
    ASSERT_EQ(origin_size, buffer.size_);
}


TEST(Buffer, ExpendUseTail)
{
    maid::Buffer buffer;
    size_t expect_size = 50;
    size_t expect_size_2 = 40;
    buffer.Expend(expect_size);
    size_t origin_size = buffer.size_;
    buffer.end += 10;
    buffer.Expend(expect_size_2);

    ASSERT_LE(expect_size, buffer.size_);
    ASSERT_EQ(buffer.base_, buffer.start);
    ASSERT_EQ(buffer.start + 10, buffer.end);
    ASSERT_EQ(origin_size, buffer.size_);

}

TEST(Buffer, ExpendUseRemain)
{
    maid::Buffer buffer;
    size_t expect_size = 50;
    size_t expect_size_2 = 40;
    buffer.Expend(expect_size);
    size_t origin_size = buffer.size_;
    buffer.start += 20;
    buffer.end += 30;
    buffer.Expend(expect_size_2);

    ASSERT_LE(expect_size, buffer.size_);
    ASSERT_EQ(buffer.base_, buffer.start);
    ASSERT_EQ(buffer.start + 10, buffer.end);
    ASSERT_EQ(origin_size, buffer.size_);
}

TEST(Buffer, ExpendUseMalloc)
{
    maid::Buffer buffer;
    size_t expect_size = 50;
    size_t expect_size_2 = 40;
    buffer.Expend(expect_size);
    size_t origin_size = buffer.size_;
    buffer.end += 50;
    buffer.Expend(expect_size_2);

    ASSERT_LE(expect_size + expect_size_2, buffer.size_);
    ASSERT_EQ(buffer.base_, buffer.start);
    ASSERT_EQ(buffer.start + 50, buffer.end);
    ASSERT_LT(origin_size, buffer.size_);
}
