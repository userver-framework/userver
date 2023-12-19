#include <userver/utils/impl/source_location.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

namespace {

auto MyFunction() {
#line 42 "my/source/file.cpp"
  return utils::impl::SourceLocation::Current();
}

}  // namespace

TEST(SourceLocation, Current) {
  const auto location = MyFunction();
  EXPECT_EQ(location.GetFileName(), "my/source/file.cpp");
  EXPECT_EQ(location.GetLine(), 42);
  EXPECT_EQ(location.GetFunctionName(), "MyFunction");
  EXPECT_EQ(location.GetLineString(), "42");
}

USERVER_NAMESPACE_END
