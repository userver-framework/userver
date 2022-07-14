#include <array>

#include <gtest/gtest.h>

#include <userver/utils/enumerate.hpp>

USERVER_NAMESPACE_BEGIN

constexpr int ConstexprTest(std::array<int, 2> data) {
  int result = 0;
  for (auto [pos, elem] : utils::enumerate(data)) {
    result += pos + elem;
  }
  return result;
}

static_assert(ConstexprTest({2, 3}) == 6,
              "Constexpr test for enumerate failed");

struct EnumerateFixture : public ::testing::Test {
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
    ContainerThrowsOnCopy(const ContainerThrowsOnCopy&) {
      throw CopyException{};
    }
    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    int* begin() { return nullptr; }
    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    int* end() { return nullptr; }
    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    const int* begin() const { return nullptr; }
    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    const int* end() const { return nullptr; }
  };

  static std::vector<int> CreateRValueData() { return {1, 2, 3, 4}; }
  static std::vector<int> CreateRValueEmpty() { return {}; }
};

TEST_F(EnumerateFixture, Vector) {
  std::vector<int> data{1, 2, 3, 4};

  int current_pos = 0;

  for (const auto [pos, elem] : utils::enumerate(data)) {
    EXPECT_EQ(pos, current_pos);
    EXPECT_EQ(elem, data[current_pos]);

    current_pos++;
  }
}

TEST_F(EnumerateFixture, EmptyVector) {
  std::vector<int> data;

  for (const auto [pos, elem] : utils::enumerate(data)) {
    std::ignore = pos;
    std::ignore = elem;
    FAIL();  // We can't get here
  }
}

TEST_F(EnumerateFixture, SingleElement) {
  std::vector<int> data;
  data.push_back(1);

  int current_pos = 0;

  for (const auto [pos, elem] : utils::enumerate(data)) {
    EXPECT_EQ(pos, current_pos);
    EXPECT_EQ(elem, data[current_pos]);

    current_pos++;
  }
}

TEST_F(EnumerateFixture, RValue) {
  std::vector<int> reference = CreateRValueData();
  int current_pos = 0;
  for (const auto [pos, elem] : utils::enumerate(CreateRValueData())) {
    EXPECT_EQ(pos, current_pos);
    EXPECT_EQ(elem, reference[current_pos]);

    current_pos++;
  }
}

TEST_F(EnumerateFixture, EmptyRValue) {
  for (const auto [pos, elem] : utils::enumerate(CreateRValueEmpty())) {
    std::ignore = pos;
    std::ignore = elem;
    FAIL();  // We can't get here
  }
}

TEST_F(EnumerateFixture, Modification) {
  std::vector<int> source{1, 2, 3};
  const std::vector<int> target{2, 3, 4};

  for (auto&& [pos, elem] : utils::enumerate(source)) {
    ++elem;
  }

  for (const auto [pos, reference_elem] : utils::enumerate(target)) {
    EXPECT_EQ(source[pos], reference_elem);
  }
}

TEST_F(EnumerateFixture, TestNoCopyLValue) {
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

TEST_F(EnumerateFixture, TestNoCopyRValue) {
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

TEST_F(EnumerateFixture, TestNoCopyElems) {
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

USERVER_NAMESPACE_END
