#include "keyshard_impl.hpp"

#include <atomic>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include <userver/utest/assert_macros.hpp>

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
    static constexpr std::size_t kThreadCount = 32;
    storages::redis::impl::KeyShardTaximeterCrc32 key_shard(kShards);

    std::vector<size_t> counts(kShards, 0);
    std::vector<std::vector<size_t>> thread_counts(kThreadCount, std::vector<size_t>(kShards, 0));
    std::atomic<size_t> count(0);

    std::vector<std::thread> threads;
    threads.reserve(kThreadCount);
    for (size_t thread_idx = 0; thread_idx < kThreadCount; ++thread_idx) {
        threads.emplace_back([&, thread_idx]() -> void {
            auto& tcounts = thread_counts[thread_idx];
            while (count++ < kCount) {
                size_t idx = 0;
                UASSERT_NO_THROW(idx = key_shard.ShardByKey(kKey));
                ++tcounts[idx];
            }
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }
    for (const auto& tcounts : thread_counts) {
        for (size_t shard_idx = 0; shard_idx < kShards; ++shard_idx) {
            counts[shard_idx] += tcounts[shard_idx];
        }
    }

    EXPECT_EQ(kCount, counts[key_shard.ShardByKey(kKey)]);
}

USERVER_NAMESPACE_END
