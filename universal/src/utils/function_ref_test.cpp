#include <userver/utils/function_ref.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

TEST(FunctionRef, Smoke) {
  auto functor = [](std::string string, std::size_t offset) {
    return string.substr(offset);
  };
  utils::function_ref<std::string(std::string, std::size_t)> ref = functor;
  EXPECT_EQ(ref("foobar", 3), "bar");

  auto* function_pointer = +functor;
  ref = function_pointer;
  EXPECT_EQ(ref("foobar", 3), "bar");

  function_pointer = nullptr;
  (void)function_pointer;  // silence "unused value" warning

  // Function pointer functors are specified to be stored by value, so the
  // original pointer is no longer required.
  EXPECT_EQ(ref("foobar", 3), "bar");
}

USERVER_NAMESPACE_END
