#include <array>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <userver/utils/enumerate.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr int ConstexprTest(std::array<int, 2> data) {
  int result = 0;
  for (auto [pos, elem] : utils::enumerate(data)) {
    result += pos + elem;
  }
  return result;
}

static_assert(ConstexprTest({2, 3}) == 6,
              "Constexpr test for enumerate failed");

class CopyException : public std::exception {
 public:
  using std::exception::exception;
};

class MoveException : public std::exception {
 public:
  using std::exception::exception;
};

struct ContainerThrowsOnCopy {
  ContainerThrowsOnCopy() = default;
  ContainerThrowsOnCopy(ContainerThrowsOnCopy&&) noexcept(false) {
    throw MoveException{};
  }
  ContainerThrowsOnCopy(const ContainerThrowsOnCopy&) { throw CopyException{}; }
  // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
  int* begin() { return nullptr; }
  // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
  int* end() { return nullptr; }
  // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
  const int* begin() const { return nullptr; }
  // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
  const int* end() const { return nullptr; }
};

std::vector<int> CreateRValueData() { return {1, 2, 3, 4}; }

std::vector<int> CreateRValueEmpty() { return {}; }

struct UntilNulSentinel final {
  [[maybe_unused]] friend bool operator==(const char* lhs, UntilNulSentinel) {
    return *lhs == '\0';
  }

  [[maybe_unused]] friend bool operator!=(const char* lhs, UntilNulSentinel) {
    return *lhs != '\0';
  }
};

struct UntilNulRange final {
  auto begin() const { return data; }
  auto end() const { return UntilNulSentinel{}; }

  const char* data;
};

}  // namespace

TEST(Enumerate, Vector) {
  std::vector<int> data{1, 2, 3, 4};

  int current_pos = 0;

  for (const auto [pos, elem] : utils::enumerate(data)) {
    EXPECT_EQ(pos, current_pos);
    EXPECT_EQ(elem, data[current_pos]);

    current_pos++;
  }
}

TEST(Enumerate, EmptyVector) {
  std::vector<int> data;

  for (const auto [pos, elem] : utils::enumerate(data)) {
    std::ignore = pos;
    std::ignore = elem;
    FAIL();  // We can't get here
  }
}

TEST(Enumerate, SingleElement) {
  std::vector<int> data;
  data.push_back(1);

  int current_pos = 0;

  for (const auto [pos, elem] : utils::enumerate(data)) {
    EXPECT_EQ(pos, current_pos);
    EXPECT_EQ(elem, data[current_pos]);

    current_pos++;
  }
}

TEST(Enumerate, RValue) {
  std::vector<int> reference = CreateRValueData();
  int current_pos = 0;
  for (const auto [pos, elem] : utils::enumerate(CreateRValueData())) {
    EXPECT_EQ(pos, current_pos);
    EXPECT_EQ(elem, reference[current_pos]);

    current_pos++;
  }
}

TEST(Enumerate, EmptyRValue) {
  for (const auto [pos, elem] : utils::enumerate(CreateRValueEmpty())) {
    std::ignore = pos;
    std::ignore = elem;
    FAIL();  // We can't get here
  }
}

TEST(Enumerate, Modification) {
  std::vector<int> source{1, 2, 3};
  const std::vector<int> target{2, 3, 4};

  for (auto&& [pos, elem] : utils::enumerate(source)) {
    ++elem;
  }

  for (const auto [pos, reference_elem] : utils::enumerate(target)) {
    EXPECT_EQ(source[pos], reference_elem);
  }
}

TEST(Enumerate, TestNoCopyLValue) {
  ContainerThrowsOnCopy container;

  try {
    for (const auto [pos, elem] : utils::enumerate(container)) {
      std::ignore = pos;
      std::ignore = elem;
      FAIL() << "container should be empty";
    }
  } catch (const CopyException& e) {
    FAIL() << "enumerate calls copy ctor for lvalue. should be only reference "
              "init.";
  } catch (const MoveException& e) {
    FAIL() << "enumerate calls move ctor for lvalue. should be only reference "
              "init.";
  }
}

TEST(Enumerate, TestNoCopyRValue) {
  auto FuncReturnContainer = []() {
    ContainerThrowsOnCopy container;
    return container;
  };

  try {
    for (const auto [pos, elem] : utils::enumerate(FuncReturnContainer())) {
      std::ignore = pos;
      std::ignore = elem;
      FAIL() << "container should be empty";
    }
  } catch (const CopyException& e) {
    FAIL() << "enumerate calls copy ctor for rvalue. should be move";
  } catch (const MoveException& e) {
    SUCCEED();
  }
}

TEST(Enumerate, TestNoCopyElems) {
  struct ThrowOnCopy {
    ThrowOnCopy() = default;
    ThrowOnCopy(const ThrowOnCopy&) { throw CopyException{}; };
  };
  std::array<ThrowOnCopy, 10> container{};
  try {
    for (auto [pos, elem] : utils::enumerate(container)) {
      std::ignore = pos;
      std::ignore = elem;
    };
  } catch (const CopyException&) {
    FAIL() << "elem is copied, it is not reference";
  }
}

TEST(Enumerate, NonCommonRange) {
  std::vector<std::pair<std::size_t, char>> result;
  auto enumerate_range = utils::enumerate(UntilNulRange{"abc"});

  // Cannot use range-for on C++17
  auto&& begin = enumerate_range.begin();
  auto&& end = enumerate_range.end();
  while (begin != end) {
    auto [pos, elem] = *begin;
    result.emplace_back(pos, elem);
    ++begin;
  }

  const std::pair<std::size_t, char> expected[] = {
      {0, 'a'},
      {1, 'b'},
      {2, 'c'},
  };
  EXPECT_THAT(result, testing::ElementsAreArray(expected));
}

USERVER_NAMESPACE_END
