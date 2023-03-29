#if 0

#include <gtest/gtest.h>

#include <algorithm>

#include <userver/cache/impl/slru.hpp>

USERVER_NAMESPACE_BEGIN

using Equal = std::equal_to<>;
using Hash = std::hash<int>;
using SLRU =
    cache::impl::LruBase<int, int, Hash, Equal, cache::CachePolicy::kSLRU>;

std::vector<int> GetProbation(SLRU& slru) {
  std::vector<int> actual;
  slru.VisitProbation([&actual](int key, int) { actual.push_back(key); });
  std::sort(actual.begin(), actual.end());
  return actual;
}

std::vector<int> GetProtected(SLRU& slru) {
  std::vector<int> actual;
  slru.VisitProtected([&actual](int key, int) { actual.push_back(key); });
  std::sort(actual.begin(), actual.end());
  return actual;
}

// TODO: decompose
TEST(SLRU, Put) {
  SLRU slru(10, std::hash<int>{}, std::equal_to<>{}, 0.8);

  for (int i = 0; i < 8; i++) EXPECT_TRUE(slru.Put(i, 0));

  EXPECT_EQ(GetProbation(slru), (std::vector<int>{0, 1, 2, 3, 4, 5, 6, 7}));
  EXPECT_EQ(slru.GetSize(), 8);

  EXPECT_TRUE(slru.Put(8, 0));
  EXPECT_EQ(GetProbation(slru), (std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8}));
  EXPECT_EQ(slru.GetSize(), 8);

  EXPECT_FALSE(slru.Put(6, 1));
  EXPECT_EQ(GetProbation(slru), (std::vector<int>{1, 2, 3, 4, 5, 7, 8}));
  EXPECT_EQ(GetProtected(slru), (std::vector<int>{6}));

  EXPECT_FALSE(slru.Put(7, 1));
  EXPECT_EQ(GetProbation(slru), (std::vector<int>{1, 2, 3, 4, 5, 8}));
  EXPECT_EQ(GetProtected(slru), (std::vector<int>{6, 7}));

  EXPECT_FALSE(slru.Put(7, 2));
  EXPECT_EQ(GetProbation(slru), (std::vector<int>{1, 2, 3, 4, 5, 8}));
  EXPECT_EQ(GetProtected(slru), (std::vector<int>{6, 7}));

  EXPECT_TRUE(slru.Put(9, 0));
  EXPECT_TRUE(slru.Put(10, 0));
  EXPECT_TRUE(slru.Put(11, 0));
  EXPECT_EQ(GetProbation(slru), (std::vector<int>{2, 3, 4, 5, 8, 9, 10, 11}));
  EXPECT_EQ(GetProtected(slru), (std::vector<int>{6, 7}));

  slru.Erase(9);
  slru.Erase(10);
  EXPECT_FALSE(slru.Put(8, 1));
  EXPECT_EQ(GetProbation(slru), (std::vector<int>{2, 3, 4, 5, 6, 11}));
  EXPECT_EQ(GetProtected(slru), (std::vector<int>{7, 8}));

  EXPECT_FALSE(slru.Put(5, 1));
  EXPECT_EQ(GetProbation(slru), (std::vector<int>{2, 3, 4, 6, 7, 11}));
  EXPECT_EQ(GetProtected(slru), (std::vector<int>{5, 8}));

  for (int i = 12; i < 20; i++) EXPECT_TRUE(slru.Put(i, 0));
  EXPECT_EQ(GetProbation(slru),
            (std::vector<int>{12, 13, 14, 15, 16, 17, 18, 19}));
  EXPECT_EQ(GetProtected(slru), (std::vector<int>{5, 8}));
}

TEST(SLRU, PutFirst) {
  SLRU slru(10, std::hash<int>{}, std::equal_to<>{}, 0.8);

  EXPECT_TRUE(slru.Put(0, 0));
  EXPECT_EQ(slru.GetSize(), 1);
}

TEST(SLRU, Get) {
  SLRU slru(10, std::hash<int>{}, std::equal_to<>{}, 0.8);
  EXPECT_FALSE(slru.Get(0));

  for (int i = 0; i < 8; i++) slru.Put(i, i);

  for (int i = 0; i < 8; i++) {
    auto* res = slru.Get(i);
    EXPECT_TRUE(res);
    EXPECT_EQ(*res, i);
  }

  EXPECT_EQ(GetProbation(slru), (std::vector<int>{0, 1, 2, 3, 4, 5}));
  EXPECT_EQ(GetProtected(slru), (std::vector<int>{6, 7}));
  slru.Erase(6);
  slru.Erase(7);
  slru.Put(6, 6);
  slru.Put(7, 7);

  slru.Put(8, 8);
  EXPECT_EQ(GetProbation(slru), (std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8}));
  EXPECT_EQ(GetProtected(slru), (std::vector<int>{}));
  EXPECT_TRUE(slru.Get(8));
  EXPECT_EQ(GetProbation(slru), (std::vector<int>{1, 2, 3, 4, 5, 6, 7}));
  EXPECT_EQ(GetProtected(slru), (std::vector<int>{8}));

  auto* res = slru.Get(1);
  EXPECT_TRUE(res);
  EXPECT_EQ(*res, 1);
  EXPECT_EQ(GetProbation(slru), (std::vector<int>{2, 3, 4, 5, 6, 7}));
  EXPECT_EQ(GetProtected(slru), (std::vector<int>{1, 8}));

  EXPECT_TRUE(slru.Get(2));
  EXPECT_EQ(GetProbation(slru), (std::vector<int>{3, 4, 5, 6, 7, 8}));
  EXPECT_EQ(GetProtected(slru), (std::vector<int>{1, 2}));
  EXPECT_TRUE(slru.Get(1));
  EXPECT_TRUE(slru.Get(2));
  EXPECT_EQ(GetProtected(slru), (std::vector<int>{1, 2}));

  slru.Put(9, 9);
  slru.Put(10, 10);
  for (int i = 3; i < 11; i++) {
    auto* res = slru.Get(i);
    EXPECT_TRUE(res);
    EXPECT_EQ(*res, i);
  }
  EXPECT_TRUE(slru.Get(1));
  EXPECT_TRUE(slru.Get(2));
  EXPECT_EQ(GetProbation(slru), (std::vector<int>{3, 4, 5, 6, 7, 8, 9, 10}));
  EXPECT_EQ(GetProtected(slru), (std::vector<int>{1, 2}));
}

TEST(SLRU, Erase) {
  SLRU slru(10, std::hash<int>{}, std::equal_to<>{}, 0.8);
  for (int i = 0; i < 8; i++) slru.Put(i, i);

  slru.Get(2);

  slru.Erase(10);
  slru.Erase(1);
  slru.Erase(2);

  EXPECT_FALSE(slru.Get(1));
  EXPECT_FALSE(slru.Get(2));
}

TEST(SLRU, Size1) {
  SLRU slru(1, std::hash<int>{}, std::equal_to<>{}, 0.8);
  slru.Put(1, 1);
  slru.Put(2, 2);
  EXPECT_EQ(GetProbation(slru), (std::vector<int>{2}));
  EXPECT_EQ(GetProtected(slru), (std::vector<int>{}));
  EXPECT_TRUE(slru.Get(2));
  EXPECT_EQ(1, slru.GetSize());
  EXPECT_FALSE(slru.Get(1));
}

USERVER_NAMESPACE_END

#endif
