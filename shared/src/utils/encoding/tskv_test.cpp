#include <algorithm>

#include <gtest/gtest.h>

#include <userver/utils/encoding/tskv.hpp>
#include <utils/encoding/tskv_testdata_bin.hpp>

USERVER_NAMESPACE_BEGIN

TEST(tskv, NoEscapeQuote) {
  const auto* str = R"({ "tasks" : [  ] })";
  for (auto mode : {utils::encoding::EncodeTskvMode::kValue,
                    utils::encoding::EncodeTskvMode::kKey}) {
    std::string result;
    utils::encoding::EncodeTskv(result, str, mode);
    EXPECT_EQ(str, result);
  }
}

TEST(tskv, TAXICOMMON1362) {
  const char* str = reinterpret_cast<const char*>(tskv_test::data_bin);
  std::string result;
  utils::encoding::EncodeTskv(result, str, sizeof(tskv_test::data_bin),
                              utils::encoding::EncodeTskvMode::kValue);
  EXPECT_TRUE(result.find("PNG") != std::string::npos) << "Result: " << result;

  EXPECT_TRUE(result.find(tskv_test::ascii_part) != std::string::npos)
      << "Result: " << result;

  EXPECT_EQ(0, std::count(result.begin(), result.end(), '\n'))
      << "Result: " << result;
  EXPECT_EQ(0, std::count(result.begin(), result.end(), '\t'))
      << "Result: " << result;
  EXPECT_EQ(0, std::count(result.begin(), result.end(), '\0'))
      << "Result: " << result;
}

USERVER_NAMESPACE_END
