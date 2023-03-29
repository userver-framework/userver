#include <gtest/gtest.h>

#include <forward_list>
#include <string>

#include <userver/utils/encoding/hex.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::encoding {

TEST(Hex, LengthInHexForm) {
  std::string empty;
  EXPECT_EQ(0, LengthInHexForm(empty));

  std::string data{"_+=156"};
  EXPECT_EQ(12, LengthInHexForm(data));

  EXPECT_EQ(6, LengthInHexForm(3));
}

TEST(Hex, FromHexUpperBound) {
  EXPECT_EQ(0, FromHexUpperBound(0));
  EXPECT_EQ(3, FromHexUpperBound(6));
  EXPECT_EQ(3, FromHexUpperBound(7));
}

TEST(Hex, ToHex) {
  constexpr std::string_view data{"_+=156"};
  constexpr std::string_view reference{"5f2b3d313536"};
  const std::string result = ToHex(data);
  EXPECT_EQ(reference, result);
}

TEST(Hex, ToHexLong) {
  constexpr std::string_view data{"21e30c92afe54396"};
  constexpr std::string_view reference{"32316533306339326166653534333936"};
  const std::string result = ToHex(data);
  EXPECT_EQ(reference, result);
}

TEST(Hex, FromHex) {
  // Test simple case - everything is correct
  {
    std::string data{"5f2b3d313536"};
    EXPECT_TRUE(IsHexData(data));
    std::string reference{"_+=156"};
    std::string result;
    result.reserve(FromHexUpperBound(data.size()));
    auto input_size = FromHex(data, result);
    EXPECT_EQ(data.size(), input_size);
    EXPECT_EQ(reference, result);
  }
  // Test missing symbol
  {
    // True data is : std::string data{"5f2b3d313536"};
    // Remove one symbol
    std::string data{"5f2b3d31353"};
    std::string reference{"_+=15"};
    std::string result;
    EXPECT_FALSE(IsHexData(data));  // no, because one symbol is missing
    // Add extra 2 elements to better check where output_last will be
    result.reserve(FromHexUpperBound(data.size()));
    auto input_size = FromHex(data, result);
    // All input except last char should have been read
    EXPECT_EQ(data.size() - 1, input_size);
    EXPECT_EQ(reference, result);
  }
  // Test wrong symbol at even position
  {
    // True data is : std::string data{"5f2b3d313536"};
    // Place wrong symbol
    std::string data{"5f2b!3d313536"};
    EXPECT_FALSE(IsHexData(data));  // no, because one symbol is not hex
    std::string reference{"_+"};
    std::string result;
    result.reserve(FromHexUpperBound(data.size()));
    auto input_size = FromHex(data, result);
    // Should have stopped at '!'
    EXPECT_NE(data.size(), input_size);
    EXPECT_EQ('!', data[input_size]);
    EXPECT_EQ(reference, result);
  }
  // Test wrong symbol at odd position
  {
    // True data is : std::string data{"5f2b3d313536"};
    // Place wrong symbol
    std::string data{"5f2b3!d313536"};
    EXPECT_FALSE(IsHexData(data));  // no, because one symbol is not hex
    std::string reference{"_+"};
    std::string result;
    result.reserve(FromHexUpperBound(data.size()));
    auto input_size = FromHex(data, result);
    // Should have stopped at '3', right before '!'
    EXPECT_NE(data.size(), input_size);
    EXPECT_EQ('3', data[input_size]);
    EXPECT_EQ(reference, result);
  }

  // Test all supported hex chars
  {
    std::string data{"0123456789abcdefABCDEF"};
    EXPECT_TRUE(IsHexData(data));
  }
}

TEST(Hex, GetHexPart) {
  // Test simple case - everything is correct
  {
    std::string data{"5f2b3d313536"};
    EXPECT_TRUE(IsHexData(data));
    EXPECT_EQ(data.size(), GetHexPart(data).size());
  }
  // Test missing symbol
  {
    // True data is : std::string data{"5f2b3d313536"};
    // Remove one symbol
    std::string data{"5f2b3d31353"};
    EXPECT_FALSE(IsHexData(data));  // no, because one symbol is missing
    EXPECT_EQ(data.size() - 1, GetHexPart(data).size());
  }
  // Test wrong symbol at even position
  {
    // True data is : std::string data{"5f2b3d313536"};
    // Place wrong symbol
    std::string data{"5f2b!3d313536"};
    EXPECT_FALSE(IsHexData(data));  // no, because one symbol is not hex
    // Should have stopped at '!'
    EXPECT_EQ(4, GetHexPart(data).size());
  }
  // Test wrong symbol at odd position
  {
    // True data is : std::string data{"5f2b3d313536"};
    // Place wrong symbol
    std::string data{"5f2b3!d313536"};
    EXPECT_EQ(4, GetHexPart(data).size());
    EXPECT_FALSE(IsHexData(data));  // no, because one symbol is not hex
  }
}

}  // namespace utils::encoding

USERVER_NAMESPACE_END
