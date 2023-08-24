#include <gtest/gtest.h>

#include <type_traits>

#include <userver/cache/lru_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace {
struct Movable {
  explicit Movable(int value) : value(value) {}

  Movable(const Movable& other) = delete;
  Movable& operator=(const Movable& other) noexcept = delete;
  Movable(Movable&& other) = default;
  Movable& operator=(Movable&& other) noexcept = default;

  int value;
};

struct NotMovable {
  explicit NotMovable(int value) : value(value) {}

  NotMovable(const NotMovable& other) = delete;
  NotMovable& operator=(const NotMovable& other) noexcept = delete;
  NotMovable(NotMovable&& other) = delete;
  NotMovable& operator=(NotMovable&& other) noexcept = delete;

  int value;
};
}  // namespace

using Lru = cache::LruMap<int, int>;

TEST(Lru, SetGet) {
  Lru cache(10);
  EXPECT_EQ(nullptr, cache.Get(1));
  cache.Put(1, 2);
  EXPECT_EQ(2, cache.GetOr(1, -1));
  cache.Put(1, 3);
  EXPECT_EQ(3, cache.GetOr(1, -1));
}

TEST(Lru, Erase) {
  Lru cache(10);
  cache.Put(1, 2);
  cache.Erase(1);
  EXPECT_EQ(nullptr, cache.Get(1));
}

TEST(Lru, MultipleSet) {
  Lru cache(10);
  cache.Put(1, 10);
  cache.Put(2, 20);
  cache.Put(3, 30);
  EXPECT_EQ(10, cache.GetOr(1, -1));
  EXPECT_EQ(20, cache.GetOr(2, -1));
  EXPECT_EQ(30, cache.GetOr(3, -1));
}

TEST(Lru, Overflow) {
  Lru cache(2);
  cache.Put(1, 10);
  cache.Put(2, 20);
  cache.Put(3, 30);
  EXPECT_EQ(-1, cache.GetOr(1, -1));
  EXPECT_EQ(20, cache.GetOr(2, -1));
  EXPECT_EQ(30, cache.GetOr(3, -1));
}

TEST(Lru, OverflowUpdate) {
  Lru cache(2);
  cache.Put(1, 10);
  cache.Put(2, 20);
  cache.Put(1, 11);
  cache.Put(3, 30);

  EXPECT_EQ(11, cache.GetOr(1, -1));
  EXPECT_EQ(-1, cache.GetOr(2, -1));
  EXPECT_EQ(30, cache.GetOr(3, -1));
}

TEST(Lru, OverflowSetMaxSizeShrink) {
  Lru cache(3);
  cache.Put(1, 10);
  cache.Put(2, 20);
  cache.Put(1, 11);
  cache.Put(3, 30);
  cache.SetMaxSize(2);

  EXPECT_EQ(11, cache.GetOr(1, -1));
  EXPECT_EQ(-1, cache.GetOr(2, -1));
  EXPECT_EQ(30, cache.GetOr(3, -1));
}

TEST(Lru, OverflowSetMaxSizeExpand) {
  Lru cache(3);
  cache.Put(1, 10);
  cache.Put(2, 20);
  cache.Put(1, 11);
  cache.Put(3, 30);
  cache.SetMaxSize(5);
  cache.Put(4, 40);
  cache.Put(5, 50);

  EXPECT_EQ(11, cache.GetOr(1, -1));
  EXPECT_EQ(20, cache.GetOr(2, -1));
  EXPECT_EQ(30, cache.GetOr(3, -1));
  EXPECT_EQ(40, cache.GetOr(4, -1));
  EXPECT_EQ(50, cache.GetOr(5, -1));
  EXPECT_EQ(-1, cache.GetOr(6, -1));

  cache.Put(6, 60);
  EXPECT_EQ(-1, cache.GetOr(1, -1));
  EXPECT_EQ(20, cache.GetOr(2, -1));
  EXPECT_EQ(30, cache.GetOr(3, -1));
  EXPECT_EQ(40, cache.GetOr(4, -1));
  EXPECT_EQ(50, cache.GetOr(5, -1));
  EXPECT_EQ(60, cache.GetOr(6, -1));
}

TEST(Lru, OverflowGet) {
  Lru cache(2);
  cache.Put(1, 10);
  cache.Put(2, 20);
  cache.Get(1);
  cache.Put(3, 30);

  EXPECT_EQ(10, cache.GetOr(1, -1));
  EXPECT_EQ(-1, cache.GetOr(2, -1));
  EXPECT_EQ(30, cache.GetOr(3, -1));
}

TEST(Lru, VisitAllConst) {
  Lru cache(10);
  cache.Put(1, 10);
  cache.Put(2, 20);
  cache.Put(3, 30);

  const auto& const_cache = cache;

  std::size_t sum_keys = 0;
  std::size_t sum_values = 0;
  // mutable - to test VisitAll compilation with non-const operator()
  const_cache.VisitAll(
      [&sum_keys, &sum_values](auto&& key, auto&& value) mutable {
        static_assert(std::is_const_v<std::remove_reference_t<decltype(key)>>,
                      "key must be const in VisitAll");
        static_assert(std::is_const_v<std::remove_reference_t<decltype(value)>>,
                      "value must be const in const VisitAll");
        sum_keys += key;
        sum_values += value;
      });

  EXPECT_EQ(6, sum_keys);
  EXPECT_EQ(60, sum_values);
}

TEST(Lru, VisitAllNonConst) {
  Lru cache(10);
  cache.Put(1, 10);
  cache.Put(2, 20);
  cache.Put(3, 30);

  cache.VisitAll([](auto&& key, auto&& value) {
    static_assert(std::is_const_v<std::remove_reference_t<decltype(key)>>,
                  "key must be const in VisitAll");
    static_assert(!std::is_const_v<std::remove_reference_t<decltype(value)>>,
                  "value must be mutable in non-const VisitAll");
    value /= 2;
  });

  EXPECT_EQ(cache.GetOr(1, -1), 5);
  EXPECT_EQ(cache.GetOr(2, -1), 10);
  EXPECT_EQ(cache.GetOr(3, -1), 15);
}

TEST(Lru, Emplace) {
  Lru cache{2};
  cache.Put(1, 10);
  EXPECT_EQ(*cache.Emplace(2, 20), 20);
  EXPECT_EQ(*cache.Get(2), 20);
  EXPECT_EQ(*cache.Emplace(1, 30), 10);
}

TEST(Lru, GetLeastUsed) {
  Lru cache{2};
  EXPECT_EQ(cache.GetLeastUsed(), nullptr);
  cache.Put(1, 10);
  cache.Put(2, 20);
  EXPECT_EQ(*cache.GetLeastUsed(), 10);
  cache.Get(1);
  EXPECT_EQ(*cache.GetLeastUsed(), 20);
}

TEST(Lru, Movable) {
  cache::LruMap<int, Movable> cache{1};

  cache.Emplace(1, 2);
  EXPECT_EQ(cache.Get(1)->value, 2);
}

TEST(Lru, NotMovable) {
  cache::LruMap<int, NotMovable> cache{2};

  cache.Emplace(1, 2);
  cache.Emplace(3, 4);
  EXPECT_EQ(cache.Get(1)->value, 2);
  EXPECT_EQ(cache.Get(3)->value, 4);
  EXPECT_EQ(cache.GetLeastUsed()->value, 2);
  cache.Emplace(5, 6);
  EXPECT_EQ(cache.GetSize(), 2);
  EXPECT_EQ(cache.GetLeastUsed()->value, 4);
}

USERVER_NAMESPACE_END
