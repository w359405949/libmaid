#include "gtest/gtest.h"
#include "buffer.h"


TEST(Buffer, Constructor)
{
    maid::Buffer buffer;

    ASSERT_EQ(0u, buffer.len);
    ASSERT_EQ(0u, buffer.total);
    ASSERT_EQ(NULL, buffer.base);
}

TEST(Buffer, ExpendSmall)
{
    maid::Buffer buffer;

    buffer.Expend(10);
    ASSERT_EQ(0u, buffer.len);
    ASSERT_LE(10u, buffer.total);
    ASSERT_TRUE(NULL != buffer.base);

    buffer.Expend(10);
    ASSERT_EQ(0u, buffer.len);
    ASSERT_LE(10u, buffer.total);
    ASSERT_TRUE(NULL != buffer.base);

    buffer.len = 10;
    buffer.Expend(10);
    ASSERT_EQ(10u, buffer.len);
    ASSERT_LE(20u, buffer.total);
    ASSERT_TRUE(NULL != buffer.base);
}


TEST(Buffer, ExpendZero)
{
    maid::Buffer buffer;

    buffer.Expend(0);
    ASSERT_EQ(0u, buffer.len);
    ASSERT_LE(0u, buffer.total);
    ASSERT_TRUE(NULL == buffer.base);
}


TEST(Buffer, ExpendLarge)
{
    maid::Buffer buffer;

    buffer.Expend(100000000);
    ASSERT_EQ(0u, buffer.len);
    ASSERT_LE(100000000u, buffer.total);
    ASSERT_TRUE(NULL != buffer.base);

    buffer.Expend(100000000);
    ASSERT_EQ(0u, buffer.len);
    ASSERT_LE(100000000u, buffer.total);
    ASSERT_TRUE(NULL != buffer.base);

    buffer.len = 100000000;
    buffer.Expend(100000000);
    ASSERT_EQ(100000000u, buffer.len);
    ASSERT_LE(200000000u, buffer.total);
    ASSERT_TRUE(NULL != buffer.base);
}

TEST(Buffer, ExpendKeepOriginDataSmall)
{
    maid::Buffer buffer;
    const char* data = "hello world";
    buffer.base = malloc(strlen(data));
    buffer.len = strlen(data);
    memmove(buffer.base, data, strlen(data) + 1);

    buffer.Expend(100);
    ASSERT_TRUE(strncmp((char*)buffer.base, data, buffer.len) == 0);
    ASSERT_LE(buffer.len, buffer.total);
    ASSERT_EQ(buffer.len, strlen(data));
}

TEST(Buffer, ExpendKeepOriginDataZero)
{
    maid::Buffer buffer;
    const char* data = "hello world";
    buffer.base = malloc(strlen(data));
    buffer.len = strlen(data);
    memmove(buffer.base, data, strlen(data) + 1);

    buffer.Expend(0);
    ASSERT_TRUE(strncmp((char*)buffer.base, data, buffer.len) == 0);
    ASSERT_LE(buffer.len, buffer.total);
    ASSERT_EQ(buffer.len, strlen(data));
}


TEST(Buffer, ExpendKeepOriginDataLarge)
{
    maid::Buffer buffer;
    const char* data = "hhello hello hello hello hello hello hello hello hello hello hello hello hello hello hello hello hello hello hello hello hello hello hello hello ello world";
    buffer.base = malloc(strlen(data));
    buffer.len = strlen(data);
    memmove(buffer.base, data, strlen(data) + 1);

    buffer.Expend(100000);
    ASSERT_TRUE(strncmp((char*)buffer.base, data, buffer.len) == 0);
    ASSERT_LE(buffer.len, buffer.total);
    ASSERT_EQ(buffer.len, strlen(data));
}
