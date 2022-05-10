#include "split_location.hpp"

#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

TEST(SplitLocation, Basic) {
  auto [path, line] = logging::SplitLocation("hello.hpp:42");
  EXPECT_EQ(path, "hello.hpp");
  EXPECT_EQ(line, 42);
}

TEST(SplitLocation, TwoDelimiters) {
  auto [path, line] = logging::SplitLocation("C:/hello.hpp:42");
  EXPECT_EQ(path, "C:/hello.hpp");
  EXPECT_EQ(line, 42);
}

TEST(SplitLocation, NoLine) {
  auto [path, line] = logging::SplitLocation("hello.hpp");
  EXPECT_EQ(path, "hello.hpp");
  EXPECT_EQ(line, logging::kAnyLine);
}

TEST(SplitLocation, WindowsPath) {
  UEXPECT_THROW_MSG(logging::SplitLocation("C:/hello.hpp"), std::exception,
                    "error");
}

USERVER_NAMESPACE_END
