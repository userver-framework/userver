#include <utest/utest.hpp>

#include <array>
#include <atomic>
#include <cstdint>

#include <engine/sleep.hpp>
#include <rcu/rcu_map.hpp>
#include <utils/async.hpp>

TEST(RcuMap, Empty) {
  RunInCoro([] {
    rcu::RcuMap<std::string, int> map;
    const auto& cmap = map;

    EXPECT_EQ(0, map.SizeApprox());
    EXPECT_EQ(map.begin(), map.end());
    EXPECT_EQ(cmap.begin(), cmap.end());
    auto snap = map.GetSnapshot();
    map.Clear();
    EXPECT_EQ(snap, map.GetSnapshot());
  });
}

TEST(RcuMap, Modify) {
  RunInCoro([] {
    rcu::RcuMap<std::string, int> map;
    const auto& cmap = map;

    EXPECT_THROW(cmap["any"], rcu::MissingKeyException);
    EXPECT_FALSE(map.Get("any"));
    EXPECT_FALSE(cmap.Get("any"));
    EXPECT_FALSE(map.Erase("any"));
    EXPECT_FALSE(map.Pop("any"));

    EXPECT_NO_THROW(*map["any"] = 1);

    EXPECT_EQ(1, *cmap["any"]);
    EXPECT_EQ(1, *map.Get("any"));
    EXPECT_EQ(1, *cmap.Get("any"));
    EXPECT_TRUE(map.Erase("any"));
    EXPECT_FALSE(map.Erase("any"));
    EXPECT_FALSE(map.Pop("any"));

    EXPECT_NO_THROW(*map["any"] = 2);
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

    EXPECT_NO_THROW(*map["any"] = 0);
    EXPECT_NO_THROW(
        map.Assign(std::unordered_map<std::string, std::shared_ptr<int>>{
            {"any", std::make_shared<int>(5)}}));
    EXPECT_EQ(*cmap["any"], 5);
    EXPECT_EQ(*map.Pop("any"), 5);
  });
}

TEST(RcuMap, Snapshot) {
  RunInCoro([] {
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
  });
}

TEST(RcuMap, ConcurrentUpdates) {
  RunInCoro(
      [] {
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

                const auto cleared_shr =
                    map[-1]->fetch_or(uint32_t{v} << (i * 8));
                ASSERT_EQ(0, cleared_shr & mask);

                *map[(i << 8) + v] = -1;
              }
              ASSERT_EQ(mask, map[-1]->fetch_and(~mask) & mask);
              ASSERT_TRUE(map.Erase((i << 8) + 0xFF));
            }
          });
        }

        engine::InterruptibleSleepFor(std::chrono::milliseconds(100));
        stop_flag = true;
        for (auto& w : workers) w.Get();

        EXPECT_TRUE(map.Erase(-1));
        EXPECT_EQ(map.begin(), map.end());
      },
      /*threads =*/4);
}

TEST(RcuMap, IterStability) {
  RunInCoro([] {
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
    for (auto& [k, v] : map) {
      *v = curr_val;
    }
    map.Erase(9);
    engine::Yield();
    map.Clear();
    rw_checker.Get();
    ro_checker.Get();
  });
}
