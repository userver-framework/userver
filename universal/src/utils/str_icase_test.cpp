#include <gtest/gtest.h>
#include <userver/utils/str_icase.hpp>

#include <algorithm>

USERVER_NAMESPACE_BEGIN

constexpr std::string_view kLowercaseChars =
    "abcdefghijklmnopqrstuvwxyz0123456789~!@#$%^&*()\n\r\t\v\b\a;:\"\'\\/,.";
constexpr std::string_view kUppercaseChars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789~!@#$%^&*()\n\r\t\v\b\a;:\"\'\\/,.";

TEST(StrIcases, Hash) {
  utils::StrIcaseHash hash{};

  const auto hash1 = hash(kLowercaseChars);
  const auto hash2 = hash(kUppercaseChars);
  EXPECT_EQ(hash1, hash2);

  const auto hash3 = hash(std::string_view{"aBcDefGhijklmnoPqrstuvwxyz"});
  const auto hash4 = hash(std::string_view{"AbCdEFgHIJKLMNOpQRSTUVWXYZ"});
  EXPECT_EQ(hash3, hash4);
  EXPECT_NE(hash1, hash3);

  const auto hash5 = hash(std::string_view("a\0BcDz0", 7));
  const auto hash6 = hash(std::string_view("A\0BcDz0", 7));
  EXPECT_EQ(hash5, hash6);
  EXPECT_NE(hash1, hash6);
  EXPECT_NE(hash3, hash6);

  EXPECT_EQ(hash(std::string_view("WARNING")),
            hash(std::string_view("warning")));
}

TEST(StrCases, Hash) {
  utils::StrCaseHash hash{};

  const auto hash1 = hash(kLowercaseChars);
  const auto hash2 = hash(kUppercaseChars);
  EXPECT_NE(hash1, hash2);
}

TEST(StrIcases, HashIsRandomlySeeded) {
  utils::StrIcaseHash hash1;
  utils::StrIcaseHash hash2;
  std::string_view value = "foo";
  EXPECT_NE(hash1(value), hash2(value));
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

TEST(StrIcases, CompareLessMany) {
  std::vector<std::string> v;
  for (size_t i = 0; i < 26; i++) {
    v.emplace_back(1, 'a' + i);
    v.emplace_back(1, 'A' + i);
  }
  // shuffle
  for (size_t i = 1; i < v.size(); i++) {
    std::swap(v[i], v[i * 311 % (i + 1)]);
  }
  std::sort(std::begin(v), std::end(v), utils::StrIcaseLess());
  std::string result;
  for (const auto& s : v) {
    result += s;
  }
  for (size_t i = 0; i < 26; i++) {
    char c1 = result[i * 2];
    char c2 = result[i * 2 + 1];
    if (c2 < c1) std::swap(c1, c2);
    EXPECT_EQ(c1, char('A' + i)) << c1 << ' ' << c2 << ' ' << result;
    EXPECT_EQ(c2, char('a' + i)) << c1 << ' ' << c2 << ' ' << result;
  }
}

TEST(StrIcases, CompareLessNonAlpha) {
  std::vector<std::string> v;
  for (size_t i = 0; i < 26; i++) {
    v.emplace_back(1, 'a' + i);
    v.emplace_back(1, 'A' + i);
  }
  v.emplace_back("@");
  v.emplace_back("[");
  v.emplace_back("\\");
  v.emplace_back("]");
  v.emplace_back("^");
  v.emplace_back("_");
  v.emplace_back("`");
  v.emplace_back("{");
  v.emplace_back("|");
  v.emplace_back("}");
  v.emplace_back("~");
  // shuffle
  for (size_t i = 1; i < v.size(); i++) {
    std::swap(v[i], v[i * 311 % (i + 1)]);
  }
  std::sort(std::begin(v), std::end(v), utils::StrIcaseLess());
  std::string result;
  for (const auto& s : v) {
    result += s;
  }
  ASSERT_EQ(result[0], '@') << result;
  ASSERT_EQ(result[1], '[') << result;
  ASSERT_EQ(result[2], '\\') << result;
  ASSERT_EQ(result[3], ']') << result;
  ASSERT_EQ(result[4], '^') << result;
  ASSERT_EQ(result[5], '_') << result;
  ASSERT_EQ(result[6], '`') << result;

  for (size_t i = 0; i < 26; i++) {
    char c1 = result[i * 2 + 7];
    char c2 = result[i * 2 + 8];
    if (c2 < c1) std::swap(c1, c2);
    ASSERT_EQ(c1, char('A' + i)) << c1 << ' ' << c2 << ' ' << result;
    ASSERT_EQ(c2, char('a' + i)) << c1 << ' ' << c2 << ' ' << result;
  }

  ASSERT_EQ(result[59], '{') << result;
  ASSERT_EQ(result[60], '|') << result;
  ASSERT_EQ(result[61], '}') << result;
  ASSERT_EQ(result[62], '~') << result;
}

USERVER_NAMESPACE_END
