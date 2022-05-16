#include <userver/utils/fixed_array.hpp>

#include <cstddef>

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

USERVER_NAMESPACE_END
