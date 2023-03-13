#include <userver/storages/redis/impl/exception.hpp>

#include <gtest/gtest.h>

#include <userver/storages/redis/impl/base.hpp>

USERVER_NAMESPACE_BEGIN

TEST(Reply, RequestFailedExceptionTimeout) {
  try {
    throw redis::RequestFailedException("descr",
                                        redis::ReplyStatus::kTimeoutError);
  } catch (const redis::RequestFailedException& ex) {
    EXPECT_TRUE(ex.IsTimeout());
    EXPECT_EQ(ex.GetStatus(), redis::ReplyStatus::kTimeoutError);
  }
}

TEST(Reply, RequestFailedException) {
  try {
    throw redis::RequestFailedException("descr",
                                        redis::ReplyStatus::kOtherError);
  } catch (const redis::RequestFailedException& ex) {
    EXPECT_FALSE(ex.IsTimeout());
    EXPECT_EQ(ex.GetStatus(), redis::ReplyStatus::kOtherError);
  }
}

USERVER_NAMESPACE_END
