#include <formats/common/utils_test.hpp>

USERVER_NAMESPACE_BEGIN

TEST(FormatsSplitPath, Empty) {
  ASSERT_TRUE(formats::common::SplitPathString("").empty());
}

TEST(FormatsSplitPath, Many) {
  std::vector<std::string> result =
      formats::common::SplitPathString("key1.key2.sample");
  ASSERT_EQ((int)(result.size()), 3);
  ASSERT_EQ(result[0], "key1");
  ASSERT_EQ(result[1], "key2");
  ASSERT_EQ(result[2], "sample");
}

USERVER_NAMESPACE_END
