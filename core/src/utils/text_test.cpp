#include <gtest/gtest.h>

#include <climits>

#include <boost/algorithm/hex.hpp>
#include <boost/algorithm/string/case_conv.hpp>

#include <userver/utest/assert_macros.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/text.hpp>

USERVER_NAMESPACE_BEGIN

using utils::text::RemoveQuotes;

namespace {

std::string ToHex(std::string_view view) {
  std::string result;
  result.reserve(view.size() * 2);
  boost::algorithm::hex(view.begin(), view.end(), std::back_inserter(result));
  boost::algorithm::to_lower(result);
  return result;
}

}  // namespace

TEST(RemoveQuotes, One) {
  EXPECT_STREQ("", RemoveQuotes("").c_str());
  EXPECT_STREQ("abc", RemoveQuotes("abc").c_str());
  EXPECT_STREQ("", RemoveQuotes("\"").c_str());
  EXPECT_STREQ("", RemoveQuotes("\"\"").c_str());
  EXPECT_STREQ("\"abc", RemoveQuotes("\"abc").c_str());
  EXPECT_STREQ("abc\"", RemoveQuotes("abc\"").c_str());
  EXPECT_STREQ("abc", RemoveQuotes("abc").c_str());
  EXPECT_STREQ("abc", RemoveQuotes("\"abc\"").c_str());
}

TEST(Text, StartsWith) {
  EXPECT_TRUE(utils::text::StartsWith("12345", "123"));
  EXPECT_TRUE(utils::text::StartsWith("12345", ""));
  EXPECT_TRUE(utils::text::StartsWith("12345", "12345"));
  EXPECT_FALSE(utils::text::StartsWith("123", "12345"));
  EXPECT_FALSE(utils::text::StartsWith("345", "12345"));
  EXPECT_FALSE(utils::text::StartsWith("123_5", "12345"));
  EXPECT_FALSE(utils::text::StartsWith("", "12345"));
  EXPECT_TRUE(utils::text::StartsWith("", ""));
}

TEST(Text, EndsWith) {
  EXPECT_TRUE(utils::text::EndsWith("12345", "345"));
  EXPECT_TRUE(utils::text::EndsWith("12345", ""));
  EXPECT_TRUE(utils::text::EndsWith("12345", "12345"));
  EXPECT_FALSE(utils::text::EndsWith("123", "12345"));
  EXPECT_FALSE(utils::text::EndsWith("345", "12345"));
  EXPECT_FALSE(utils::text::EndsWith("123_5", "12345"));
  EXPECT_FALSE(utils::text::EndsWith("", "12345"));
  EXPECT_TRUE(utils::text::EndsWith("", ""));
}

TEST(Text, SplitSV) {
  {
    std::string input = "1,22,333";
    std::vector<std::string_view> tokens =
        utils::text::SplitIntoStringViewVector(input, ",");
    ASSERT_EQ(input.data(), tokens[0].data());
    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0], "1");
    EXPECT_EQ(tokens[1], "22");
    EXPECT_EQ(tokens[2], "333");
  }
  {
    std::string input;
    std::vector<std::string_view> tokens =
        utils::text::SplitIntoStringViewVector(input, ",");
    ASSERT_EQ(tokens.size(), 1);
  }
  {
    std::string input = ",";
    std::vector<std::string_view> tokens =
        utils::text::SplitIntoStringViewVector(input, ",");
    ASSERT_EQ(tokens.size(), 2);
  }
  {
    std::string input = ",,";
    std::vector<std::string_view> tokens =
        utils::text::SplitIntoStringViewVector(input, ",");
    ASSERT_EQ(tokens.size(), 3);
  }
  {
    std::string input = ",0,";
    std::vector<std::string_view> tokens =
        utils::text::SplitIntoStringViewVector(input, ",");
    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0], "");
    EXPECT_EQ(tokens[1], "0");
    EXPECT_EQ(tokens[2], "");
  }
}

TEST(TestIsAscii, IsAscii) {
  EXPECT_TRUE(utils::text::IsAscii("valid ascii"));

  EXPECT_FALSE(utils::text::IsAscii("\xa0\xa1"));
}

TEST(TestIsUtf8, IsUtf8) {
  EXPECT_TRUE(utils::text::IsUtf8("hello world"));

  EXPECT_TRUE(utils::text::IsUtf8("ol\xc3\xa1"));

  EXPECT_TRUE(utils::text::IsUtf8("{\"order\": {\"comment\": \"test\"}}"));

  EXPECT_TRUE(utils::text::IsUtf8("{\"comment\": \"\u041c\u043e\u0441\"}"));

  EXPECT_TRUE(utils::text::IsUtf8("\u041c\u043e\u0441\u043a\u0432\u0430"));

  EXPECT_TRUE(utils::text::IsUtf8("\xf4\x80\x83\x92"));
  EXPECT_TRUE(utils::text::IsUtf8("\xed\x80\xbf"));

  EXPECT_FALSE(utils::text::IsUtf8("\xa0\xa1"));

  EXPECT_FALSE(utils::text::IsUtf8("\xf0\x28\x8c\xbc"));

  EXPECT_FALSE(utils::text::IsUtf8("\xc0\xaf"));

  EXPECT_FALSE(utils::text::IsUtf8("\xe0\x9f\x80"));
}

TEST(TestTrimUtf8Truncated, TrimTruncatedEnding) {
  auto test_trim = [](std::string test_str, const std::string expected) {
    auto test_str_orig = test_str;

    auto test_view_str = test_str;
    auto test_view = std::string_view{test_view_str};

    UEXPECT_NO_THROW(utils::text::utf8::TrimTruncatedEnding(test_str))
        << "hex(test_str_orig): " << ToHex(test_str_orig);
    EXPECT_EQ(test_str, expected)
        << "hex(test_str_orig): " << ToHex(test_str_orig);

    UEXPECT_NO_THROW(utils::text::utf8::TrimViewTruncatedEnding(test_view))
        << "hex(test_str_orig): " << ToHex(test_str_orig);
    EXPECT_EQ(test_view, std::string_view{expected})
        << "hex(test_str_orig): " << ToHex(test_str_orig);

    // have to check, that TrimViewTruncatedEnding
    // did not change the original string
    EXPECT_EQ(test_view_str, test_str_orig);
  };

  test_trim("", "");
  test_trim("hello world", "hello world");

  test_trim("ol\xc3\xa1", "ol\xc3\xa1");
  test_trim("ol\xc3", "ol");
  test_trim("ol\xc3\xa1\xc3", "ol\xc3\xa1");
  test_trim("\u041c\u043e\u0441\u043a\u0432\u0430",
            "\u041c\u043e\u0441\u043a\u0432\u0430");
  test_trim("\xf4\x80\x83\x92", "\xf4\x80\x83\x92");
  test_trim("\xf4\x80\x83\x92\xf4\x80\x83", "\xf4\x80\x83\x92");
  test_trim("\xf4\x80\x83\x92\xf4\x80", "\xf4\x80\x83\x92");
  test_trim("\xf4\x80\x83\x92\xf4", "\xf4\x80\x83\x92");
  test_trim("\xf4\x80\x83", "");
  test_trim("\xed\x80\xbf", "\xed\x80\xbf");
  test_trim("\xed\x80\xbf\xed\x80", "\xed\x80\xbf");
  test_trim("\xed\x80\xbf\xed", "\xed\x80\xbf");

  // some not utf-8 strings
  test_trim("\xa0\xa1", "\xa0\xa1");
  test_trim("\xa0", "\xa0");
  test_trim("\xf0\x28\x8c\xbc", "\xf0\x28\x8c\xbc");
  test_trim("\xc0\xaf", "\xc0\xaf");

  // TrimTruncatedEnding does not validate all string characters
  test_trim("\xe0\x9f\xc3", "\xe0\x9f");
}

TEST(TextUtils, CamelCaseToSnake) {
  using utils::text::CamelCaseToSnake;

  EXPECT_STREQ("", CamelCaseToSnake("").c_str());
  EXPECT_STREQ("a", CamelCaseToSnake("A").c_str());
  EXPECT_STREQ("_", CamelCaseToSnake("_").c_str());
  EXPECT_STREQ("-", CamelCaseToSnake("-").c_str());
  EXPECT_STREQ("a_b_c_d", CamelCaseToSnake("ABCD").c_str());
  EXPECT_STREQ("__a", CamelCaseToSnake("_A").c_str());
  EXPECT_STREQ(" _ab_f/_bcd_e-_kq_ ",
               CamelCaseToSnake(" AbF/BcdE-Kq_ ").c_str());
}

TEST(CheckTextTest, Ascii) {
  using utils::text::IsPrintable;

  EXPECT_FALSE(IsPrintable(std::string(1, '\0')));
  EXPECT_FALSE(IsPrintable("\x01"));
  EXPECT_FALSE(IsPrintable("\x1f"));
  EXPECT_FALSE(IsPrintable("\x7f"));

  EXPECT_TRUE(IsPrintable("abcxyzABC0123XYZ_;#\x20\x7e"));
  EXPECT_FALSE(IsPrintable("\xa0\xa1"));
}

TEST(CheckTextTest, Utf8) {
  using utils::text::IsPrintable;

  EXPECT_FALSE(IsPrintable(std::string(1, '\0'), false));
  EXPECT_FALSE(IsPrintable("\x01", false));
  EXPECT_FALSE(IsPrintable("\x1f", false));
  EXPECT_FALSE(IsPrintable("\x7f", false));

  EXPECT_TRUE(IsPrintable("abcxyzABC0123XYZ_;#\x20\x7e", false));
  EXPECT_TRUE(IsPrintable("Абвгд", false));
  EXPECT_FALSE(IsPrintable("\xa0\xa1", false));
}

TEST(GetCodePointsCountTest, All) {
  using utils::text::utf8::GetCodePointsCount;
  const std::string bad(1, static_cast<char>(0xff));

  ASSERT_EQ(0U, GetCodePointsCount(""));
  UEXPECT_THROW(GetCodePointsCount(bad + "anton"), std::runtime_error);
  ASSERT_EQ(5U, GetCodePointsCount("anton"));
  ASSERT_EQ(5U, GetCodePointsCount("Антон"));
  ASSERT_EQ(11U, GetCodePointsCount("Антон anton"));
  UEXPECT_THROW(GetCodePointsCount("Ант" + bad + "он"), std::runtime_error);
}

TEST(CheckIsCString, IsCString) {
  using utils::text::IsCString;

  EXPECT_TRUE(IsCString("test"));

  std::string all_chars;
  for (int i = 1; i < (1 << CHAR_BIT); ++i) {
    all_chars.push_back(i);
  }
  EXPECT_TRUE(IsCString(all_chars));

  EXPECT_FALSE(IsCString(std::string("\0", 1)));
  EXPECT_FALSE(IsCString(std::string("a\0", 2)));
  EXPECT_FALSE(IsCString(std::string("\0a", 2)));
  EXPECT_FALSE(IsCString(std::string("a\0b", 3)));
  EXPECT_FALSE(IsCString(std::string("a\0\0b", 4)));
  EXPECT_FALSE(IsCString(std::string("a\0b\0", 4)));
  EXPECT_FALSE(IsCString(std::string("\0a\0b\0", 5)));
}

TEST(GetTextPosByCodePointPosTakePrefixRemovePrefix, Utf8) {
  const auto& DoCheck = [](const std::string_view text,    //
                           const std::size_t count,        //
                           const std::size_t text_pos,     //
                           const std::string_view prefix,  //
                           const std::string_view suffix) {
    using utils::text::utf8::GetTextPosByCodePointPos;
    ASSERT_EQ(text_pos, GetTextPosByCodePointPos(text, count));

    {
      auto temp = std::string{text};
      utils::text::utf8::TakePrefix(temp, count);
      ASSERT_EQ(prefix, temp);
    }

    {
      auto temp = text;
      utils::text::utf8::TakeViewPrefix(temp, count);
      ASSERT_EQ(prefix, temp);
    }

    {
      auto temp = std::string{text};
      utils::text::utf8::RemovePrefix(temp, count);
      ASSERT_EQ(suffix, temp);
    }

    {
      auto temp = text;
      utils::text::utf8::RemoveViewPrefix(temp, count);
      ASSERT_EQ(suffix, temp);
    }
  };

  // NOLINTNEXTLINE(readability-redundant-string-init)
  constexpr std::string_view kEmptyText{""};
  DoCheck(kEmptyText, 0, 0, "", "");
  DoCheck(kEmptyText, 1, 0, "", "");

  constexpr std::string_view kSimpleText{"abc"};
  DoCheck(kSimpleText, 0, 0, "", "abc");
  DoCheck(kSimpleText, 1, 1, "a", "bc");
  DoCheck(kSimpleText, 2, 2, "ab", "c");
  DoCheck(kSimpleText, 3, 3, "abc", "");
  DoCheck(kSimpleText, 4, 3, "abc", "");

  constexpr std::string_view kComplexText{"aЭ百Юb度科dЯ"};
  DoCheck(kComplexText, 0, 0, "", "aЭ百Юb度科dЯ");
  DoCheck(kComplexText, 1, 1, "a", "Э百Юb度科dЯ");
  DoCheck(kComplexText, 2, 3, "aЭ", "百Юb度科dЯ");
  DoCheck(kComplexText, 3, 6, "aЭ百", "Юb度科dЯ");
  DoCheck(kComplexText, 4, 8, "aЭ百Ю", "b度科dЯ");
  DoCheck(kComplexText, 5, 9, "aЭ百Юb", "度科dЯ");
  DoCheck(kComplexText, 6, 12, "aЭ百Юb度", "科dЯ");
  DoCheck(kComplexText, 7, 15, "aЭ百Юb度科", "dЯ");
  DoCheck(kComplexText, 8, 16, "aЭ百Юb度科d", "Я");
  DoCheck(kComplexText, 9, 18, "aЭ百Юb度科dЯ", "");
  DoCheck(kComplexText, 10, 18, "aЭ百Юb度科dЯ", "");
  DoCheck(kComplexText, 99, 18, "aЭ百Юb度科dЯ", "");

  // invalid utf-8
  DoCheck("\xF7", 1, 1, "\xF7", "");
  DoCheck("\xC2_bcdef", 1, 2, "\xC2_", "bcdef");
  DoCheck("\xE0_bcdef", 1, 3, "\xE0_b", "cdef");
  DoCheck("\xF0_bcdef", 1, 4, "\xF0_bc", "def");
  DoCheck("\xFF_bcdef", 1, 4, "\xFF_bc", "def");
  DoCheck("\xFF_bcdef", 2, 5, "\xFF_bcd", "ef");
  DoCheck("\xE0_b\xC0", 2, 4, "\xE0_b\xC0", "");
}

UTEST(TextNumberFormat, ArabicLocale) {
  using utils::text::Format;
  EXPECT_EQ(Format(123.45, "ar", 10, false), "123.45");
  EXPECT_EQ(Format(123.45, "ru", 10, false), "123,45");
}

USERVER_NAMESPACE_END
