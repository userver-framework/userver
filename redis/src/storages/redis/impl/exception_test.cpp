#include <userver/storages/redis/impl/exception.hpp>

#include <gtest/gtest.h>

#include <userver/storages/redis/impl/base.hpp>

USERVER_NAMESPACE_BEGIN

TEST(Reply, RequestFailedExceptionTimeout) {
  try {
    throw redis::RequestFailedException("descr", redis::REDIS_ERR_TIMEOUT);
  } catch (const redis::RequestFailedException& ex) {
    EXPECT_TRUE(ex.IsTimeout());
    EXPECT_EQ(ex.GetStatus(), redis::REDIS_ERR_TIMEOUT);
  }
}

TEST(Reply, RequestFailedExceptionNotReady) {
  try {
    throw redis::RequestFailedException("descr", redis::REDIS_ERR_NOT_READY);
  } catch (const redis::RequestFailedException& ex) {
    EXPECT_FALSE(ex.IsTimeout());
    EXPECT_EQ(ex.GetStatus(), redis::REDIS_ERR_NOT_READY);
  }
}

USERVER_NAMESPACE_END
