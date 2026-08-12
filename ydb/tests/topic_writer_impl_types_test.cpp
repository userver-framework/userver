#include <gtest/gtest.h>
#include <ydb/impl/topic_writer_impl_types.hpp>

USERVER_NAMESPACE_BEGIN

TEST(TopicWriterWorkerState, IsAtomic) {
    std::atomic<ydb::impl::TopicWriterWorkerState> atomic{ydb::impl::TopicWriterWorkerState::kInitial};
    EXPECT_TRUE(atomic.is_lock_free());
}

USERVER_NAMESPACE_END
