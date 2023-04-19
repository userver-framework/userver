#include <userver/rcu/rcu_map.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <future>
#include <mutex>
#include <thread>

#include <userver/engine/sleep.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

template <typename Key, typename Value>
struct RcuTraitsStdMutex : rcu::DefaultRcuMapTraits<Key, Value> {
  using MutexType = std::mutex;
};

using StdMutexRcuMap =
    rcu::RcuMap<std::string, int, RcuTraitsStdMutex<std::string, int>>;

}  // namespace

TEST(RcuMap, StdMutexBase) {
  StdMutexRcuMap map;
  const auto& cmap = map;

  UEXPECT_THROW(cmap["any"], rcu::MissingKeyException);
  EXPECT_FALSE(map.Get("any"));
  EXPECT_FALSE(cmap.Get("any"));
  EXPECT_FALSE(map.Erase("any"));
  EXPECT_FALSE(map.Pop("any"));

  UEXPECT_NO_THROW(*map["any"] = 1);
}

TEST(RcuMap, StdMutexConcurentWrites) {
  StdMutexRcuMap map;
  std::atomic<bool> thread_started_write{false};

  auto write_ptr = map.StartWrite();
  auto thread = std::async([&map, &thread_started_write] {
    auto write_ptr = map.StartWrite();
    thread_started_write.store(true);
  });
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_FALSE(thread_started_write.load());
  write_ptr.Commit();

  thread.get();
  ASSERT_TRUE(thread_started_write.load());
}

UTEST(RcuMap, Empty) {
  rcu::RcuMap<std::string, int> map;
  const auto& cmap = map;

  EXPECT_EQ(0, map.SizeApprox());
  EXPECT_EQ(map.begin(), map.end());
  EXPECT_EQ(cmap.begin(), cmap.end());
  auto snap = map.GetSnapshot();
  map.Clear();
  EXPECT_EQ(snap, map.GetSnapshot());
}

UTEST(RcuMap, Modify) {
  rcu::RcuMap<std::string, int> map;
  const auto& cmap = map;

  UEXPECT_THROW(cmap["any"], rcu::MissingKeyException);
  EXPECT_FALSE(map.Get("any"));
  EXPECT_FALSE(cmap.Get("any"));
  EXPECT_FALSE(map.Erase("any"));
  EXPECT_FALSE(map.Pop("any"));

  UEXPECT_NO_THROW(*map["any"] = 1);

  EXPECT_EQ(1, *cmap["any"]);
  EXPECT_EQ(1, *map.Get("any"));
  EXPECT_EQ(1, *cmap.Get("any"));
  EXPECT_TRUE(map.Erase("any"));
  EXPECT_FALSE(map.Erase("any"));
  EXPECT_FALSE(map.Pop("any"));

  UEXPECT_NO_THROW(*map["any"] = 2);
  EXPECT_EQ(2, *cmap["any"]);
  EXPECT_EQ(2, *map.Get("any"));
  EXPECT_EQ(2, *cmap.Get("any"));
  EXPECT_EQ(2, *map.Pop("any"));
  EXPECT_FALSE(map.Erase("any"));
  EXPECT_FALSE(map.Pop("any"));

  EXPECT_TRUE(map.Insert("any", std::make_shared<int>(3)).inserted);
  EXPECT_FALSE(map.Insert("any", std::make_shared<int>(0)).inserted);
  EXPECT_EQ(*map.Insert("any", std::make_shared<int>(0)).value, 3);
  EXPECT_EQ(*cmap["any"], 3);
  EXPECT_EQ(*map.Pop("any"), 3);

  EXPECT_TRUE(map.Emplace("any", 4).inserted);
  EXPECT_FALSE(map.Emplace("any", 0).inserted);
  EXPECT_EQ(*map.Emplace("any", 0).value, 4);
  EXPECT_EQ(*cmap["any"], 4);
  EXPECT_EQ(*map.Pop("any"), 4);

  EXPECT_TRUE(map.TryEmplace("any", 4).inserted);
  EXPECT_FALSE(map.TryEmplace("any", 0).inserted);
  EXPECT_EQ(*map.TryEmplace("any", 0).value, 4);
  EXPECT_EQ(*cmap["any"], 4);
  EXPECT_EQ(*map.Pop("any"), 4);

  UEXPECT_NO_THROW(*map["any"] = 0);
  UEXPECT_NO_THROW(
      map.Assign(std::unordered_map<std::string, std::shared_ptr<int>>{
          {"any", std::make_shared<int>(5)}}));
  EXPECT_EQ(*cmap["any"], 5);
  EXPECT_EQ(*map.Pop("any"), 5);
}

UTEST(RcuMap, InsertOrAssign) {
  rcu::RcuMap<std::string, int> map;

  map.InsertOrAssign("foo", std::make_shared<int>(10));
  EXPECT_EQ(*map["foo"], 10);

  map.InsertOrAssign("foo", std::make_shared<int>(20));
  EXPECT_EQ(*map["foo"], 20);
}

UTEST(RcuMap, Snapshot) {
  rcu::RcuMap<std::string, int> map;

  const auto empty_snap = map.GetSnapshot();
  EXPECT_TRUE(empty_snap.empty());

  *map["a"] = 1;

  const auto first_snap = map.GetSnapshot();
  EXPECT_TRUE(empty_snap.empty());
  EXPECT_TRUE(first_snap.count("a"));
  EXPECT_TRUE(first_snap.at("a"));
  EXPECT_EQ(1, *first_snap.at("a"));

  *map["a"] = *map["b"] = 2;

  const auto second_snap = map.GetSnapshot();
  EXPECT_TRUE(empty_snap.empty());
  EXPECT_TRUE(first_snap.count("a"));
  EXPECT_TRUE(first_snap.at("a"));
  EXPECT_EQ(2, *first_snap.at("a"));
  EXPECT_TRUE(second_snap.count("a"));
  EXPECT_TRUE(second_snap.at("a"));
  EXPECT_EQ(2, *second_snap.at("a"));
  EXPECT_TRUE(second_snap.count("b"));
  EXPECT_TRUE(second_snap.at("b"));
  EXPECT_EQ(2, *second_snap.at("b"));
}

UTEST_MT(RcuMap, ConcurrentUpdates, 4) {
  rcu::RcuMap<int, std::atomic<uint32_t>> map;
  std::array<engine::TaskWithResult<void>, 4> workers;
  std::atomic<bool> stop_flag{false};

  for (size_t i = 0; i < workers.size(); ++i) {
    workers[i] = utils::Async("writer", [i, &map, &stop_flag] {
      const uint32_t mask = 0xFFu << (i * 8);
      while (!stop_flag) {
        *map[i << 8] = -1;
        for (uint8_t v = 1; v != 0; ++v) {
          ASSERT_TRUE(map.Get((i << 8) + v - 1));

          const auto prev_shr = map[-1]->fetch_and(~mask);
          ASSERT_EQ(v - 1, (prev_shr & mask) >> (i * 8));

          ASSERT_TRUE(map.Erase((i << 8) + v - 1));

          const auto cleared_shr = map[-1]->fetch_or(uint32_t{v} << (i * 8));
          ASSERT_EQ(0, cleared_shr & mask);

          *map[(i << 8) + v] = -1;
        }
        ASSERT_EQ(mask, map[-1]->fetch_and(~mask) & mask);
        ASSERT_TRUE(map.Erase((i << 8) + 0xFF));
      }
    });
  }

  engine::SleepFor(std::chrono::milliseconds(100));
  stop_flag = true;
  for (auto& w : workers) w.Get();

  EXPECT_TRUE(map.Erase(-1));
  EXPECT_EQ(map.begin(), map.end());
}

UTEST_MT(RcuMap, ConcurrentTryEmplace, 16) {
  const size_t kReps = 100;

  for (size_t rep = 0; rep < kReps; rep++) {
    rcu::RcuMap<std::string, int> map;

    const size_t kTasks = 16;
    std::atomic<size_t> insertions = 0;

    std::vector<engine::TaskWithResult<void>> tasks;
    for (size_t i = 0; i < kTasks; i++) {
      tasks.push_back(engine::AsyncNoSpan([&map, &insertions, i] {
        auto key = std::string(20 + i / 2, 'x');
        auto res = map.TryEmplace(key, i);
        if (res.inserted) ++insertions;
        EXPECT_EQ(*res.value / 2, i / 2);
      }));
    }
    for (auto& task : tasks) {
      task.Get();
    }
    EXPECT_EQ(insertions, kTasks / 2);
  }
}

UTEST(RcuMap, IterStability) {
  rcu::RcuMap<int, int> map;
  const auto& cmap = map;
  std::atomic<int> curr_val{1};

  for (int i = 0; i < 10; ++i) {
    *map[i] = curr_val;
  }

  std::atomic<int> started_count{0};
  auto check = [&](auto&& m) {
    bool has_this_started = false;
    std::array<bool, 10> seen{};
    for (const auto& [k, v] : m) {
      if (!std::exchange(has_this_started, true)) ++started_count;

      ASSERT_TRUE(k >= 0 && k < static_cast<int>(seen.size()));
      EXPECT_FALSE(std::exchange(seen[k], true));
      EXPECT_EQ(curr_val, *v);
      engine::Yield();
    }
  };

  auto rw_checker = utils::Async("rw_checker", [&] { check(map); });
  auto ro_checker = utils::Async("ro_checker", [&] { check(cmap); });

  while (started_count < 2) engine::Yield();

  curr_val = 2;
  for (const auto& [k, v] : map) {
    *v = curr_val;
  }
  map.Erase(9);
  engine::Yield();
  map.Clear();
  rw_checker.Get();
  ro_checker.Get();
}

UTEST(RcuMap, SampleRcuMapVariable) {
  /// [Sample rcu::RcuMap usage]
  struct Data {
    // Access to RcuMap content must be synchronized via std::atomic
    // or other synchronization primitives
    std::atomic<int> x{0};
    std::atomic<bool> flag{false};
  };
  rcu::RcuMap<std::string, Data> map;

  // If the key is not in the dictionary,
  // then a default object will be created
  map["123"]->x++;
  map["other_data"]->flag = true;
  ASSERT_EQ(map["123"]->x.load(), 1);
  ASSERT_EQ(map["123"]->flag.load(), false);
  ASSERT_EQ(map["other_data"]->x.load(), 0);
  ASSERT_EQ(map["other_data"]->flag.load(), true);
  /// [Sample rcu::RcuMap usage]
}

UTEST(RcuMap, MapOfConst) {
  rcu::RcuMap<std::string, const int> map;
  map.Emplace("foo", 10);
  map.Emplace("bar", 20);
  EXPECT_EQ(*map["foo"], 10);
  EXPECT_EQ(*map["bar"], 20);

  // Values are immutable
  // *map["foo"] = 42;

  int value_sum = 0;
  for (const auto& [key, value] : map) {
    value_sum += *value;
  }
  EXPECT_EQ(value_sum, 30);
}

UTEST(RcuMap, StartWriteNoTearing) {
  using Map = rcu::RcuMap<std::string, int>;
  Map map;

  auto checker = engine::AsyncNoSpan([&] {
    Map::Snapshot snapshot;
    while (true) {
      snapshot = map.GetSnapshot();
      if (!snapshot.empty()) break;
    }
    EXPECT_EQ(snapshot.size(), 2);
    EXPECT_EQ(*snapshot.at("foo"), 10);
    EXPECT_EQ(*snapshot.at("bar"), 20);
  });

  auto txn = map.StartWrite();
  txn->emplace("foo", std::make_shared<int>(10));
  txn->emplace("bar", std::make_shared<int>(20));
  txn.Commit();

  UEXPECT_NO_THROW(checker.Get());
}

USERVER_NAMESPACE_END
