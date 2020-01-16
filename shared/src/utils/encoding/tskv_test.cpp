#include <gtest/gtest.h>

#include <utils/encoding/tskv.hpp>

TEST(tskv, NoEscapeQuote) {
  const auto str = "{ \"tasks\" : [  ] }";
  for (auto mode : {utils::encoding::EncodeTskvMode::kValue,
                    utils::encoding::EncodeTskvMode::kKey}) {
    std::string result;
    utils::encoding::EncodeTskv(result, str, mode);
    EXPECT_EQ(str, result);
  }
}
