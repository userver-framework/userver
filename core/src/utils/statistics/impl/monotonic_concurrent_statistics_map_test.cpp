#include "monotonic_concurrent_statistics_map.hpp"

#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include <gmock/gmock.h>

#include <userver/utest/utest.hpp>
#include <userver/utils/statistics/labels.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using utils::statistics::impl::StatisticsKey;
using utils::statistics::impl::StatisticsKeyView;

std::atomic<std::ptrdiff_t> test_values_alive{0};

struct TestValue {
    int value{0};

    TestValue() { ++test_values_alive; }
    explicit TestValue(int v)
        : value(v)
    {
        ++test_values_alive;
    }
    ~TestValue() { --test_values_alive; }
    TestValue(TestValue&&) = delete;
    TestValue& operator=(TestValue&&) = delete;
};

/// Helper to build StatisticsKeyView with owning label storage.
struct KeyBuilder {
    std::string path;
    std::vector<utils::statistics::Label> labels;

    void Add(std::string name, std::string value) { labels.emplace_back(std::move(name), std::move(value)); }

    StatisticsKeyView Build() {
        thread_local std::vector<utils::statistics::LabelView> views;
        views.clear();
        for (const auto& l : labels) {
            views.push_back(static_cast<utils::statistics::LabelView>(l));
        }
        return StatisticsKeyView{path, utils::statistics::LabelsSpan{views.data(), views.data() + views.size()}};
    }
};

}  // namespace

UTEST(MonotonicConcurrentStatisticsMap, Empty) {
    utils::statistics::impl::MonotonicConcurrentStatisticsMap<TestValue> map;
    KeyBuilder kb;
    kb.path = "path";
    kb.Add("a", "1");
    EXPECT_FALSE(map.Find(kb.Build()).has_value());
    EXPECT_EQ(test_values_alive.load(), 0);
}

UTEST(MonotonicConcurrentStatisticsMap, BasicEmplace) {
    {
        utils::statistics::impl::MonotonicConcurrentStatisticsMap<TestValue> map;
        KeyBuilder kb;
        kb.path = "path";
        kb.Add("a", "1");
        auto key = kb.Build();

        auto [val1, inserted1] = map.TryEmplace(key, 42);
        EXPECT_TRUE(inserted1);
        EXPECT_EQ(val1.value, 42);
        EXPECT_EQ(test_values_alive.load(), 1);

        auto [val2, inserted2] = map.TryEmplace(key, 99);
        EXPECT_FALSE(inserted2);
        EXPECT_EQ(val2.value, 42);
        EXPECT_EQ(test_values_alive.load(), 1);
    }
    EXPECT_EQ(test_values_alive.load(), 0);
}

UTEST(MonotonicConcurrentStatisticsMap, BasicFind) {
    utils::statistics::impl::MonotonicConcurrentStatisticsMap<TestValue> map;
    KeyBuilder kb1;
    kb1.path = "path1";
    kb1.Add("a", "1");
    KeyBuilder kb2;
    kb2.path = "path2";
    kb2.Add("b", "2");

    map.TryEmplace(kb1.Build(), 1);
    map.TryEmplace(kb2.Build(), 2);

    auto found1 = map.Find(kb1.Build());
    ASSERT_TRUE(found1.has_value());
    EXPECT_EQ(found1->value, 1);

    auto found2 = map.Find(kb2.Build());
    ASSERT_TRUE(found2.has_value());
    EXPECT_EQ(found2->value, 2);

    KeyBuilder kb3;
    kb3.path = "path3";
    kb3.Add("c", "3");
    EXPECT_FALSE(map.Find(kb3.Build()).has_value());
}

UTEST(MonotonicConcurrentStatisticsMap, Rehashing) {
    {
        utils::statistics::impl::MonotonicConcurrentStatisticsMap<TestValue> map(4);

        for (int i = 0; i < 20; ++i) {
            KeyBuilder kb;
            kb.path = "path";
            kb.Add("i", std::to_string(i));
            auto [val, inserted] = map.TryEmplace(kb.Build(), i);
            EXPECT_TRUE(inserted);
            EXPECT_EQ(val.value, i);
        }

        for (int i = 0; i < 20; ++i) {
            KeyBuilder kb;
            kb.path = "path";
            kb.Add("i", std::to_string(i));
            auto found = map.Find(kb.Build());
            ASSERT_TRUE(found.has_value());
            EXPECT_EQ(found->value, i);
        }

        EXPECT_EQ(test_values_alive.load(), 20);
    }
    EXPECT_EQ(test_values_alive.load(), 0);
}

UTEST(MonotonicConcurrentStatisticsMap, ConstFind) {
    utils::statistics::impl::MonotonicConcurrentStatisticsMap<TestValue> map;
    KeyBuilder kb;
    kb.path = "path";
    kb.Add("a", "1");
    map.TryEmplace(kb.Build(), 42);

    const auto& const_map = map;
    auto found = const_map.Find(kb.Build());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->value, 42);
}

UTEST(MonotonicConcurrentStatisticsMap, Visit) {
    utils::statistics::impl::MonotonicConcurrentStatisticsMap<TestValue> map;
    KeyBuilder kb1;
    KeyBuilder kb2;
    KeyBuilder kb3;
    kb1.path = "p1";
    kb1.Add("a", "1");
    kb2.path = "p2";
    kb2.Add("b", "2");
    kb3.path = "p3";
    kb3.Add("c", "3");
    map.TryEmplace(kb1.Build(), 1);
    map.TryEmplace(kb2.Build(), 2);
    map.TryEmplace(kb3.Build(), 3);

    std::vector<int> visited_values;
    map.Visit([&visited_values](const StatisticsKey&, TestValue& val) {
        visited_values.push_back(val.value);
        val.value += 100;
    });

    EXPECT_THAT(visited_values, ::testing::UnorderedElementsAre(1, 2, 3));

    auto found1 = map.Find(kb1.Build());
    ASSERT_TRUE(found1.has_value());
    EXPECT_EQ(found1->value, 101);
}

UTEST(MonotonicConcurrentStatisticsMap, StressTest) {
    constexpr int kNumThreads = 4;
    constexpr int kItemsPerThread = 100;
    constexpr auto kTestDuration = std::chrono::milliseconds(500);

    const auto deadline = std::chrono::steady_clock::now() + kTestDuration;

    while (std::chrono::steady_clock::now() < deadline) {
        EXPECT_EQ(test_values_alive.load(), 0);
        utils::statistics::impl::MonotonicConcurrentStatisticsMap<TestValue> map(4);

        std::atomic<bool> start_flag{false};
        std::atomic<int> ready_count{0};
        std::vector<std::thread> threads;
        threads.reserve(kNumThreads);

        for (int thread_id = 0; thread_id < kNumThreads; ++thread_id) {
            threads.emplace_back([&map, &start_flag, &ready_count, thread_id]() {
                ready_count.fetch_add(1, std::memory_order_relaxed);
                while (!start_flag.load(std::memory_order_acquire)) {
                    // Spin wait.
                }

                for (int i = 0; i < kItemsPerThread; ++i) {
                    int key_val = (thread_id * kItemsPerThread) + i;
                    KeyBuilder kb;
                    kb.path = "path_" + std::to_string(thread_id);
                    kb.Add("i", std::to_string(key_val));
                    auto [val, inserted] = map.TryEmplace(kb.Build(), key_val);
                    EXPECT_TRUE(inserted);
                    EXPECT_EQ(val.value, key_val);
                }
            });
        }

        while (ready_count.load(std::memory_order_relaxed) < kNumThreads) {
            // Spin wait
        }

        start_flag.store(true, std::memory_order_release);

        for (auto& thread : threads) {
            thread.join();
        }

        for (int thread_id = 0; thread_id < kNumThreads; ++thread_id) {
            for (int i = 0; i < kItemsPerThread; ++i) {
                int key_val = (thread_id * kItemsPerThread) + i;
                KeyBuilder kb;
                kb.path = "path_" + std::to_string(thread_id);
                kb.Add("i", std::to_string(key_val));
                auto found = map.Find(kb.Build());
                ASSERT_TRUE(found.has_value()) << "Missing key: path_" << thread_id << " i=" << key_val;
                EXPECT_EQ(found->value, key_val);
            }
        }

        EXPECT_EQ(test_values_alive.load(), kNumThreads * kItemsPerThread);
    }
}

USERVER_NAMESPACE_END
