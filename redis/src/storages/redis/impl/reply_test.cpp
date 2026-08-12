#include <userver/storages/redis/reply.hpp>

#include <iterator>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

static_assert(std::input_iterator<storages::redis::ReplyData::MovableKeyValues::Iterator>);

TEST(Reply, MovableKeyValuesIterator) {
    storages::redis::ReplyData data{storages::redis::ReplyData::Array{
        storages::redis::ReplyData{"key1"},
        storages::redis::ReplyData{"value1"},
        storages::redis::ReplyData{"key2"},
        storages::redis::ReplyData{"value2"},
    }};

    auto key_values = data.GetMovableKeyValues();
    ASSERT_EQ(key_values.size(), 2);

    auto it = key_values.begin();
    EXPECT_EQ(it, key_values.begin());
    EXPECT_NE(it, key_values.end());

    auto view0 = *it;
    EXPECT_EQ(view0.Key(), "key1");
    EXPECT_EQ(view0.Value(), "value1");

    auto prev = it++;
    EXPECT_EQ((*prev).Key(), "key1");
    EXPECT_EQ((*it).Key(), "key2");
    EXPECT_EQ((*it).Value(), "value2");

    auto assigned = key_values.begin();
    assigned = it;
    EXPECT_EQ(assigned, it);

    it++;
    EXPECT_EQ(it, key_values.end());
}

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
