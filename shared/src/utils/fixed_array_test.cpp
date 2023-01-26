#include <userver/utils/fixed_array.hpp>

#include <atomic>
#include <cstdint>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

TEST(FixedArray, Sample) {
  /// [Sample FixedArray]
  class NonDefaultConstructible final {
   public:
    explicit NonDefaultConstructible(std::string str) : str_(std::move(str)) {}

    std::string GetString() const { return str_; }

   private:
    std::string str_;
  };

  utils::FixedArray<NonDefaultConstructible> array{42, "The string"};
  ASSERT_EQ(array.size(), 42);
  ASSERT_EQ(array[0].GetString(), "The string");
  ASSERT_EQ(array[14].GetString(), "The string");
  ASSERT_EQ(array[41].GetString(), "The string");
  /// [Sample FixedArray]
}

TEST(FixedArray, Iteration) {
  utils::FixedArray<int> array(4, 2);
  int sum = 0;
  for (int value : array) {
    sum += value;
  }
  ASSERT_EQ(sum, 8);
}

TEST(FixedArray, Overaligned) {
  constexpr std::size_t align = alignof(std::max_align_t) * 8;
  struct Overaligned {
    alignas(align) char c;
  };
  utils::FixedArray<Overaligned> array(4);
  ASSERT_TRUE((reinterpret_cast<std::uintptr_t>(array.data()) % align) == 0);
}

TEST(FixedArray, Empty) {
  utils::FixedArray<int> array(0, 42);
  EXPECT_EQ(array.size(), 0);
  EXPECT_TRUE(array.empty());
  EXPECT_EQ(array.begin(), array.end());
}

TEST(FixedArray, GenerateEmpty) {
  auto array = utils::GenerateFixedArray(0, [](int x) { return x; });
  static_assert(std::is_same_v<decltype(array), utils::FixedArray<int>>);
  EXPECT_EQ(array.size(), 0);
  EXPECT_TRUE(array.empty());
  EXPECT_EQ(array.begin(), array.end());
}

TEST(FixedArray, GenerateOrder) {
  constexpr std::size_t kObjectCount = 42;

  std::size_t iteration_index = 0;
  auto array = utils::GenerateFixedArray(kObjectCount, [&](std::size_t index) {
    EXPECT_EQ(index, iteration_index++);
    return index * 2;
  });

  EXPECT_EQ(array.size(), kObjectCount);
  EXPECT_FALSE(array.empty());
  std::size_t index = 0;
  for (const auto& item : array) {
    EXPECT_EQ(item, index++ * 2);
  }
  EXPECT_EQ(index, kObjectCount);
}

TEST(FixedArray, GenerateNonMovable) {
  using NonMovable = std::atomic<int>;
  constexpr std::size_t kObjectCount = 42;

  auto array = utils::GenerateFixedArray(kObjectCount,
                                         [](int x) { return NonMovable(x); });

  EXPECT_EQ(array.size(), kObjectCount);
  EXPECT_FALSE(array.empty());
  std::size_t index = 0;
  for (const auto& item : array) {
    EXPECT_EQ(item.load(), index++);
  }
  EXPECT_EQ(index, kObjectCount);
}

USERVER_NAMESPACE_END
