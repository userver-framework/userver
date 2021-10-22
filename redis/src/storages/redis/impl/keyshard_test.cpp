#include "keyshard_impl.hpp"

#include <gtest/gtest.h>
#include <atomic>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

USERVER_NAMESPACE_BEGIN

const size_t kCount = 10000;
const size_t kShards = 16;
const std::string kKey =
    "Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index."
    "КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:"
    "Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index."
    "КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:"
    "Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index."
    "КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:"
    "Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index."
    "КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:"
    "Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index."
    "КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:"
    "Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index."
    "КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:"
    "Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index."
    "КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:"
    "Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index."
    "КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:"
    "Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index."
    "КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:"
    "Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index."
    "КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:"
    "Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index."
    "КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:"
    "Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index."
    "КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:"
    "Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index."
    "КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:"
    "Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index."
    "КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:"
    "Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index."
    "КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:"
    "Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index."
    "КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:"
    "Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index."
    "КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:"
    "Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index."
    "КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:"
    "Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index."
    "КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:"
    "Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index."
    "КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:"
    "Index.КАЗАХСТАН:Index.КАЗАХСТАН:Index.КАЗАХСТАН:";

TEST(KeyShardTaximeterCrc32, Multithreads) {
  redis::KeyShardTaximeterCrc32 key_shard(kShards);

  boost::mutex mutex;
  std::vector<size_t> counts(kShards, 0);
  std::atomic<size_t> count(0);

  boost::thread_group threads;
  for (size_t i = 0; i < 32; ++i) {
    threads.create_thread([&]() -> void {
      std::vector<size_t> tcounts(kShards, 0);
      while (count++ < kCount) {
        size_t idx = 0;
        ASSERT_NO_THROW(idx = key_shard.ShardByKey(kKey));
        ++tcounts[idx];
      }

      boost::mutex::scoped_lock guard(mutex);
      for (size_t i = 0; i < kShards; ++i) counts[i] += tcounts[i];
    });
  }
  threads.join_all();

  EXPECT_EQ(kCount, counts[key_shard.ShardByKey(kKey)]);
}

USERVER_NAMESPACE_END
