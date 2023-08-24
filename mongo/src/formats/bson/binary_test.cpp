#include <gtest/gtest.h>

#include <thread>

#include <userver/formats/bson.hpp>
#include <userver/utest/assert_macros.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

const formats::bson::Document kEmptyDoc;
const std::string kEmptyDocBinary{5, 0, 0, 0, 0};

const auto kTimePoint =
    std::chrono::system_clock::from_time_t(0) + std::chrono::milliseconds{1};

const auto kDoc = formats::bson::MakeDoc(
    "string", "test", "int", int64_t{1}, "double", 1.0, "inf",
    std::numeric_limits<double>::infinity(), "date", kTimePoint);

// clang-format off
const std::string kDocBinary{
    78, 0, 0, 0,
      0x02, 's', 't', 'r', 'i', 'n', 'g', '\0',
        5, 0, 0, 0, 't', 'e', 's', 't', '\0',
      0x12, 'i', 'n', 't', '\0',
        1, 0, 0, 0, 0, 0, 0, 0,
      0x01, 'd', 'o', 'u', 'b', 'l', 'e', '\0',
        0, 0, 0, 0, 0, 0, static_cast<char>(0xF0), 0x3F,
      0x01, 'i', 'n', 'f', '\0',
        0, 0, 0, 0, 0, 0, static_cast<char>(0xF0), 0x7F,
      0x09, 'd', 'a', 't', 'e', '\0',
        1, 0, 0, 0, 0, 0, 0, 0,
      0x00};
// clang-format on

}  // namespace

TEST(Binary, Empty) {
  EXPECT_EQ(kEmptyDocBinary,
            formats::bson::ToBinaryString(kEmptyDoc).ToString());
  EXPECT_EQ(kEmptyDoc, formats::bson::FromBinaryString(kEmptyDocBinary));
}

TEST(Binary, Smoke) {
  EXPECT_EQ(kDocBinary, formats::bson::ToBinaryString(kDoc).ToString());
  EXPECT_EQ(kDoc, formats::bson::FromBinaryString(kDocBinary));
}

TEST(Binary, Concurrent) {
  constexpr unsigned kValues = 50000;

  std::vector<formats::bson::Value> binary_values;
  binary_values.reserve(kValues);
  for (unsigned i = 0; i < kValues; ++i) {
    binary_values.push_back(formats::bson::FromBinaryString(kDocBinary));
  }

  auto loop_through = [&binary_values]() {
    for (auto& v : binary_values) {
      for (const auto& [key, value] : Items(v)) {
        EXPECT_TRUE(key.size() >= 3);
      }
    }
  };

  std::thread t1(loop_through);
  loop_through();
  t1.join();
}

TEST(Binary, Invalid) {
  // Invalid size/missing terminator
  UEXPECT_THROW(formats::bson::FromBinaryString(std::string{}),
                formats::bson::ParseException);
  UEXPECT_THROW(formats::bson::FromBinaryString(std::string{'\x00'}),
                formats::bson::ParseException);
  UEXPECT_THROW(formats::bson::FromBinaryString(std::string{'\x01'}),
                formats::bson::ParseException);
  UEXPECT_THROW(formats::bson::FromBinaryString(std::string{4, 0, 0, 0}),
                formats::bson::ParseException);
  UEXPECT_THROW(formats::bson::FromBinaryString(std::string{5, 0, 0, 0}),
                formats::bson::ParseException);

  // Invalid size
  UEXPECT_THROW(formats::bson::FromBinaryString(std::string{
                    5, 0, 0, 0, 0x0A, 't', 'e', 's', 't', '\0', 0x00}),
                formats::bson::ParseException);

  // Empty key
  UEXPECT_THROW(formats::bson::FromBinaryString(
                    std::string{7, 0, 0, 0, 0x7F, '\0', 0x00}),
                formats::bson::ParseException);

  // Invalid boolean value
  UEXPECT_THROW(formats::bson::FromBinaryString(std::string{
                    12, 0, 0, 0, 0x08, 't', 'e', 's', 't', '\0', 2, 0x00}),
                formats::bson::ParseException);

#ifndef USERVER_FEATURE_NO_PATCHED_BSON
  // Trailing data
  UEXPECT_THROW(
      formats::bson::FromBinaryString(std::string{6, 0, 0, 0, 0x00, 0x00}),
      formats::bson::ParseException);

  // Invalid subdocument size
  UEXPECT_THROW(
      formats::bson::FromBinaryString(std::string{
          // clang-format off
                   15, 0, 0, 0,
                     0x03, 't', 'e', 's', 't', '\0',
                       4, 0, 0, 0,
                     0x00
          // clang-format on
      }),
      formats::bson::ParseException);
#endif
}

USERVER_NAMESPACE_END
