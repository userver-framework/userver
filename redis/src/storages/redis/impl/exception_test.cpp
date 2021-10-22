#include <userver/storages/redis/impl/exception.hpp>

#include <gtest/gtest.h>

#include <userver/storages/redis/impl/base.hpp>

USERVER_NAMESPACE_BEGIN

using namespace redis;

TEST(Reply, RequestFailedExceptionTimeout) {
  try {
    throw RequestFailedException("descr", REDIS_ERR_TIMEOUT);
  } catch (const RequestFailedException& ex) {
    EXPECT_TRUE(ex.IsTimeout());
    EXPECT_EQ(ex.GetStatus(), REDIS_ERR_TIMEOUT);
  }
}

TEST(Reply, RequestFailedExceptionNotReady) {
  try {
    throw RequestFailedException("descr", REDIS_ERR_NOT_READY);
  } catch (const RequestFailedException& ex) {
    EXPECT_FALSE(ex.IsTimeout());
    EXPECT_EQ(ex.GetStatus(), REDIS_ERR_NOT_READY);
  }
}

USERVER_NAMESPACE_END
