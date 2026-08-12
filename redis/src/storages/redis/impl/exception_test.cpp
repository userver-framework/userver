#include <userver/storages/redis/exception.hpp>

#include <gtest/gtest.h>

#include <userver/storages/redis/base.hpp>

USERVER_NAMESPACE_BEGIN

TEST(Reply, RequestFailedExceptionTimeout) {
    try {
        throw storages::redis::RequestFailedException("descr", storages::redis::ReplyStatus::kTimeoutError);
    } catch (const storages::redis::RequestFailedException& ex) {
        EXPECT_TRUE(ex.IsTimeout());
        EXPECT_EQ(ex.GetStatus(), storages::redis::ReplyStatus::kTimeoutError);
    }
}

TEST(Reply, RequestFailedException) {
    try {
        throw storages::redis::RequestFailedException("descr", storages::redis::ReplyStatus::kOtherError);
    } catch (const storages::redis::RequestFailedException& ex) {
        EXPECT_FALSE(ex.IsTimeout());
        EXPECT_EQ(ex.GetStatus(), storages::redis::ReplyStatus::kOtherError);
    }
}

USERVER_NAMESPACE_END
