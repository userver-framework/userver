#include <userver/storages/redis/reply.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

TEST(Reply, IsUnusableInstanceErrorMASTERDOWN) {
    auto data = storages::redis::ReplyData::CreateError(
        "MASTERDOWN Link with MASTER is down and slave-serve-stale-data is set "
        "to 'no'."
    );
    EXPECT_TRUE(data.IsUnusableInstanceError());
}

TEST(Reply, IsUnusableInstanceErrorLOADING) {
    auto data = storages::redis::ReplyData::CreateError("LOADING Redis is loading the dataset in memory");
    EXPECT_TRUE(data.IsUnusableInstanceError());
}

TEST(Reply, IsUnusableInstanceErrorERR) {
    auto data = storages::redis::ReplyData::CreateError("ERR index out of range");
    EXPECT_FALSE(data.IsUnusableInstanceError());
}

USERVER_NAMESPACE_END
