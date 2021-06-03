#include <gtest/gtest.h>
#include <utils/str_icase.hpp>

constexpr std::string_view kLowercaseChars =
    "abcdefghijklmnopqrstuvwxyz0123456789~!@#$%^&*()\n\r\t\v\b\a;:\"\'\\/,.";
constexpr std::string_view kUppercaseChars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789~!@#$%^&*()\n\r\t\v\b\a;:\"\'\\/,.";

TEST(StrIcases, Hash) {
  const auto hash1 = utils::StrIcaseHash{}(kLowercaseChars);
  const auto hash2 = utils::StrIcaseHash{}(kUppercaseChars);
  EXPECT_EQ(hash1, hash2);

  const auto hash3 =
      utils::StrIcaseHash{}(std::string_view{"aBcDefGhijklmnoPqrstuvwxyz"});
  const auto hash4 =
      utils::StrIcaseHash{}(std::string_view{"AbCdEFgHIJKLMNOpQRSTUVWXYZ"});
  EXPECT_EQ(hash3, hash4);
  EXPECT_NE(hash1, hash3);

  const auto hash5 = utils::StrIcaseHash{}(std::string_view("a\0BcDz0", 7));
  const auto hash6 = utils::StrIcaseHash{}(std::string_view("A\0BcDz0", 7));
  EXPECT_EQ(hash5, hash6);
  EXPECT_NE(hash1, hash6);
  EXPECT_NE(hash3, hash6);

  EXPECT_EQ(utils::StrIcaseHash{}(std::string_view("WARNING")),
            utils::StrIcaseHash{}(std::string_view("warning")));
}

TEST(StrIcases, CompareEqual) {
  EXPECT_TRUE(utils::StrIcaseEqual{}(kLowercaseChars, kUppercaseChars));
  EXPECT_TRUE(utils::StrIcaseEqual{}(kUppercaseChars, kLowercaseChars));
  EXPECT_TRUE(
      utils::StrIcaseEqual{}(std::string_view{"abcdefghijklmnopqrstuvwxyz"},
                             std::string_view{"aBcDefGhijklmnoPqrstuvwxyz"}));
  EXPECT_TRUE(
      utils::StrIcaseEqual{}(std::string_view{"abcdefghijklmnopqrstuvwxyz"},
                             std::string_view{"AbCdEFgHIJKLMNOpQRSTUVWXYZ"}));
  EXPECT_TRUE(utils::StrIcaseEqual{}(
      std::string_view{"0123456789abcdefghijklmnopqrstuvwXYZ"},
      std::string_view{"0123456789AbCdEFgHIJKLMNOpQRSTUVWxyz"}));

  EXPECT_FALSE(utils::StrIcaseEqual{}(std::string_view("\0", 1),
                                      std::string_view("\32", 1)));
  EXPECT_FALSE(utils::StrIcaseEqual{}(std::string_view("\1", 1),
                                      std::string_view("\33", 1)));
  EXPECT_FALSE(utils::StrIcaseEqual{}(std::string_view("\2", 1),
                                      std::string_view("\34", 1)));
  EXPECT_FALSE(utils::StrIcaseEqual{}(std::string_view("\3", 1),
                                      std::string_view("\35", 1)));
  EXPECT_FALSE(utils::StrIcaseEqual{}(std::string_view("\4", 1),
                                      std::string_view("\36", 1)));
  EXPECT_FALSE(utils::StrIcaseEqual{}(std::string_view("\5", 1),
                                      std::string_view("\37", 1)));
  EXPECT_FALSE(utils::StrIcaseEqual{}(std::string_view("\6", 1),
                                      std::string_view("\38", 1)));

  EXPECT_TRUE(utils::StrIcaseEqual{}(std::string_view("WARNING"),
                                     std::string_view("warning")));

  EXPECT_FALSE(utils::StrIcaseEqual{}(std::string_view("WARNIN"),
                                      std::string_view("warning")));
  EXPECT_FALSE(utils::StrIcaseEqual{}(std::string_view("WARNING"),
                                      std::string_view("warnin")));
}

TEST(StrIcases, CompareLess) {
  EXPECT_FALSE(utils::StrIcaseLess{}(kUppercaseChars, kLowercaseChars));
  EXPECT_FALSE(utils::StrIcaseLess{}(kLowercaseChars, kUppercaseChars));

  EXPECT_FALSE(
      utils::StrIcaseLess{}(std::string_view{"abcdefghijklmnopqrstuvwxyz"},
                            std::string_view{"aBcDefGhijklmnoPqrstuvwxyz"}));
  EXPECT_FALSE(
      utils::StrIcaseLess{}(std::string_view{"abcdefghijklmnopqrstuvwxyz"},
                            std::string_view{"AbCdEFgHIJKLMNOpQRSTUVWXYZ"}));

  EXPECT_TRUE(utils::StrIcaseLess{}(std::string_view("WARNIN"),
                                    std::string_view("warning")));
  EXPECT_TRUE(utils::StrIcaseLess{}(std::string_view("warnin"),
                                    std::string_view("WARNING")));

  EXPECT_TRUE(utils::StrIcaseLess{}(std::string_view("\0a", 2),
                                    std::string_view("\0b", 2)));
  EXPECT_FALSE(utils::StrIcaseLess{}(std::string_view("\0b", 2),
                                     std::string_view("\0a", 2)));
  EXPECT_TRUE(utils::StrIcaseLess{}(std::string_view("\0bc", 3),
                                    std::string_view("abc", 3)));
  EXPECT_TRUE(utils::StrIcaseLess{}(std::string_view("\0bc", 3),
                                    std::string_view("ab", 2)));
}
