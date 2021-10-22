#include <gtest/gtest.h>

#include <string>

#include <userver/utils/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

std::string TruncatedMsg(const std::string& data, size_t bytes) {
  return data + "...(truncated, total " + std::to_string(bytes) + " bytes)";
}

std::string NonUtf8Msg(size_t bytes) {
  return "<Non utf-8, total " + std::to_string(bytes) + " bytes>";
}

const std::string kBinaryData = std::string("\x01\x02\0\x03\x04\x05", 6);
const std::string kBinaryHex = "010200030405";

const std::string kValidUtf8Data = "qwerty";
const std::string kBrokenUtf8Data =
    std::string("qwe") + static_cast<char>(0b10111111) + std::string("rty");

const std::string kMultibyteUtf8Data = "qweрти";

}  // namespace

TEST(TestToLimitedHex, NonTruncated) {
  EXPECT_EQ(utils::log::ToLimitedHex(kBinaryData, 30), kBinaryHex);
}

TEST(TestToLimitedHex, Truncated) {
  // if "limit" is odd number, then "(limit-1)" will be used
  // e.g.: if limit = 7, then limit = 6 will be used
  auto data = utils::log::ToLimitedHex(kBinaryData, 7);
  auto correct_data = TruncatedMsg(kBinaryHex.substr(0, 6), kBinaryData.size());
  EXPECT_EQ(data, correct_data);
}

TEST(TestToLimitedHex, TruncatedAll) {
  auto data = utils::log::ToLimitedHex(kBinaryData, 0);
  auto correct_data = TruncatedMsg("", kBinaryData.size());
  EXPECT_EQ(data, correct_data);
}

TEST(TestToLimitedHex, TruncatedNegative) {
  EXPECT_EQ(utils::log::ToLimitedHex(kBinaryData, -1), kBinaryHex);
}

TEST(TestToLimitedHex, Empty) {
  EXPECT_EQ(utils::log::ToLimitedHex("", 0), "");
}

TEST(TestToLimitedUtf8, ValidNonTruncated) {
  EXPECT_EQ(utils::log::ToLimitedUtf8(kValidUtf8Data, 30), kValidUtf8Data);
}

TEST(TestToLimitedUtf8, ValidTruncated) {
  auto data = utils::log::ToLimitedUtf8(kValidUtf8Data, 3);
  auto correct_data =
      TruncatedMsg(kValidUtf8Data.substr(0, 3), kValidUtf8Data.size());
  EXPECT_EQ(data, correct_data);
}

TEST(TestToLimitedUtf8, ValidTruncatedAll) {
  auto data = utils::log::ToLimitedUtf8(kValidUtf8Data, 0);
  auto correct_data = TruncatedMsg("", kValidUtf8Data.size());
  EXPECT_EQ(data, correct_data);
}

TEST(TestToLimitedUtf8, ValidTruncatedNegative) {
  EXPECT_EQ(utils::log::ToLimitedUtf8(kValidUtf8Data, -1), kValidUtf8Data);
}

TEST(TestToLimitedUtf8, BrokenNonTruncated) {
  auto data = utils::log::ToLimitedUtf8(kBrokenUtf8Data, 30);
  auto correct_data = NonUtf8Msg(kBrokenUtf8Data.size());
  EXPECT_EQ(data, correct_data);
}

TEST(TestToLimitedUtf8, BrokenTruncatedBad) {
  auto data = utils::log::ToLimitedUtf8(kBrokenUtf8Data, 4);
  auto correct_data = NonUtf8Msg(kBrokenUtf8Data.size());
  EXPECT_EQ(data, correct_data);
}

TEST(TestToLimitedUtf8, BrokenTruncatedGood) {
  auto data = utils::log::ToLimitedUtf8(kBrokenUtf8Data, 3);
  auto correct_data =
      TruncatedMsg(kBrokenUtf8Data.substr(0, 3), kBrokenUtf8Data.size());
  EXPECT_EQ(data, correct_data);
}

TEST(TestToLimitedUtf8, BrokenTruncatedAll) {
  auto data = utils::log::ToLimitedUtf8(kBrokenUtf8Data, 0);
  auto correct_data = TruncatedMsg("", kBrokenUtf8Data.size());
  EXPECT_EQ(data, correct_data);
}

TEST(TestToLimitedUtf8, BrokenTruncatedNegative) {
  auto data = utils::log::ToLimitedUtf8(kBrokenUtf8Data, -1);
  auto correct_data = NonUtf8Msg(kBrokenUtf8Data.size());
  EXPECT_EQ(data, correct_data);
}

TEST(TestToLimitedUtf8, MultibyteNonTruncated) {
  EXPECT_EQ(utils::log::ToLimitedUtf8(kMultibyteUtf8Data, 30),
            kMultibyteUtf8Data);
}

TEST(TestToLimitedUtf8, MultibyteTruncatedSingle) {
  auto data = utils::log::ToLimitedUtf8(kMultibyteUtf8Data, 3);
  auto correct_data =
      TruncatedMsg(kMultibyteUtf8Data.substr(0, 3), kMultibyteUtf8Data.size());
  EXPECT_EQ(data, correct_data);
}

TEST(TestToLimitedUtf8, MultibyteTruncatedFullCodePoint) {
  auto data = utils::log::ToLimitedUtf8(kMultibyteUtf8Data, 7);
  auto correct_data =
      TruncatedMsg(kMultibyteUtf8Data.substr(0, 7), kMultibyteUtf8Data.size());
  EXPECT_EQ(data, correct_data);
}

TEST(TestToLimitedUtf8, MultibyteTruncatedHalfCodePoint) {
  auto data = utils::log::ToLimitedUtf8(kMultibyteUtf8Data, 6);
  auto correct_data =
      TruncatedMsg(kMultibyteUtf8Data.substr(0, 5), kMultibyteUtf8Data.size());
  EXPECT_EQ(data, correct_data);
}

TEST(TestToLimitedUtf8, MultibyteTruncatedAll) {
  auto data = utils::log::ToLimitedUtf8(kMultibyteUtf8Data, 0);
  auto correct_data = TruncatedMsg("", kMultibyteUtf8Data.size());
  EXPECT_EQ(data, correct_data);
}

TEST(TestToLimitedUtf8, MultibyteTruncatedNegative) {
  EXPECT_EQ(utils::log::ToLimitedUtf8(kMultibyteUtf8Data, -1),
            kMultibyteUtf8Data);
}

TEST(TestToLimitedUtf8, Empty) {
  EXPECT_EQ(utils::log::ToLimitedUtf8("", 0), "");
}

USERVER_NAMESPACE_END
