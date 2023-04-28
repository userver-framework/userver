#include <userver/cache/impl/slru.hpp>

#include <string>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

TEST(SlruBase, Sample) {
  constexpr std::size_t probation_part = 40;
  constexpr std::size_t protected_part = 10;
  cache::impl::SlruBase<std::string, int> cache(probation_part, protected_part);

  for (std::string str = "a"; str.size() < 10; str.push_back('a')) {
    for (std::size_t i = 0; i < 4; ++i) {
      cache.Put(str, str.size());
    }
  }

  for (std::string str = "b"; str.size() < 100; str.push_back('b')) {
    cache.Put(str, str.size());
  }

  for (std::string str = "a"; str.size() < 10; str.push_back('a')) {
    EXPECT_EQ(str.size(), *cache.Get(str));
  }
}

TEST(SlruBase, ProbationPart) {
  constexpr std::size_t probation_part = 40;
  constexpr std::size_t protected_part = 10;
  cache::impl::SlruBase<std::size_t, std::size_t> cache(probation_part,
                                                        protected_part);

  for (std::size_t i = 0; i < 1000; ++i) {
    cache.Put(i, i);
  }
  std::size_t cache_hit = 0;
  for (std::size_t i = 0; i < 1000; ++i) {
    if (cache.Get(i)) {
      cache_hit++;
    }
  }
  EXPECT_LE(probation_part, cache_hit);
}

TEST(SlruBase, ProtectedPart) {
  constexpr std::size_t probation_part = 40;
  constexpr std::size_t protected_part = 10;
  cache::impl::SlruBase<std::size_t, std::size_t> cache(probation_part,
                                                        protected_part);

  for (std::size_t i = 0; i < protected_part; ++i) {
    for (std::size_t j = 0; j < 10; ++j) {
      cache.Put(i, i);
    }
  }

  for (std::size_t i = 0; i < 100 * probation_part; ++i) {
    cache.Put(1000 + i, 1000 + i);
  }

  for (std::size_t i = 0; i < protected_part; ++i) {
    EXPECT_EQ(i, *cache.Get(i));
  }
}

TEST(SlruBase, NodeManipulation) {
  constexpr std::size_t probation_part = 40;
  constexpr std::size_t protected_part = 10;
  cache::impl::SlruBase<std::size_t, std::size_t> cache1(probation_part,
                                                         protected_part);
  cache::impl::SlruBase<std::size_t, std::size_t> cache2(probation_part,
                                                         protected_part);

  for (std::size_t i = 0; i < 10; i += 2) {
    cache1.Put(i, i);
  }
  for (std::size_t i = 1; i < 10; i += 2) {
    cache2.Put(i, i);
  }

  for (std::size_t i = 0; i < 10; i += 2) {
    auto node = cache1.ExtractNode(i);
    cache2.InsertNode(std::move(node));
  }
  for (std::size_t i = 1; i < 10; i += 2) {
    auto node = cache2.ExtractNode(i);
    cache1.InsertNode(std::move(node));
  }

  for (std::size_t i = 0; i < 10; i += 2) {
    EXPECT_EQ(i, *cache2.Get(i));
  }
  for (std::size_t i = 1; i < 10; i += 2) {
    EXPECT_EQ(i, *cache1.Get(i));
  }
}

TEST(SlruBase, FullProtectedPart) {
  constexpr std::size_t probation_part = 40;
  constexpr std::size_t protected_part = 10;
  cache::impl::SlruBase<std::size_t, std::size_t> cache(probation_part,
                                                        protected_part);

  for (std::size_t i = 0; i < protected_part; ++i) {
    for (std::size_t j = 0; j < 10; ++j) {
      cache.Put(i, i);
    }
  }
  for (std::size_t i = 0; i < 100 * probation_part; ++i) {
    cache.Put(1000 + i, 1000 + i);
  }
  for (std::size_t i = protected_part; i < 2 * protected_part; ++i) {
    for (std::size_t j = 0; j < 10; ++j) {
      cache.Put(i, i);
    }
  }

  for (std::size_t i = 0; i < 2 * protected_part; ++i) {
    EXPECT_EQ(i, *cache.Get(i));
  }
}

TEST(SlruBase, Erase) {
  constexpr std::size_t probation_part = 40;
  constexpr std::size_t protected_part = 10;
  cache::impl::SlruBase<std::size_t, std::size_t> cache(probation_part,
                                                        protected_part);

  for (std::size_t i = 0; i < 10; ++i) {
    for (std::size_t j = 0; j < 10; ++j) {
      cache.Put(i, i);
    }
    cache.Put(1000 + i, 1000 + i);
  }

  for (std::size_t i = 0; i < 10; ++i) {
    cache.Erase(i);
    cache.Erase(1000 + i);
  }

  for (std::size_t i = 0; i < 10; ++i) {
    EXPECT_EQ(nullptr, cache.Get(i));
    EXPECT_EQ(nullptr, cache.Get(1000 + i));
  }
}

TEST(SlruBase, LeastUsed) {
  constexpr std::size_t probation_part = 40;
  constexpr std::size_t protected_part = 10;
  cache::impl::SlruBase<std::size_t, std::size_t> cache(probation_part,
                                                        protected_part);

  for (std::size_t i = 0; i < 30; ++i) {
    cache.Put(i, i);
  }
  EXPECT_EQ(0, *cache.GetLeastUsedKey());
  EXPECT_EQ(0, *cache.GetLeastUsedValue());

  cache.Put(0, 0);

  for (std::size_t i = 1; i < 1000; ++i) {
    cache.Put(i, i);
  }
  EXPECT_NE(0, *cache.GetLeastUsedKey());
  EXPECT_NE(0, *cache.GetLeastUsedValue());
}

USERVER_NAMESPACE_END
