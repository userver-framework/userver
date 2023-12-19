#include <userver/utest/utest.hpp>
#include <userver/utils/text.hpp>

USERVER_NAMESPACE_BEGIN

UTEST(TextNumberFormat, ArabicLocale) {
  using utils::text::Format;
  EXPECT_EQ(Format(123.45, "ar", 10, false), "123.45");
  EXPECT_EQ(Format(123.45, "ru", 10, false), "123,45");
}

UTEST(TextToUpper, EnLocale) {
  using utils::text::ToUpper;
  EXPECT_EQ(ToUpper("teSt", "en"), "TEST");
}

UTEST(TextNumberFormat, NoLocale) {
  using utils::text::Format;
  EXPECT_EQ(Format(123.12345, 8), "123.12345");
  EXPECT_EQ(Format(123.12345, 5), "123.12");
}

UTEST(TextFormat, ToLower) {
  using utils::text::ToLower;
  EXPECT_EQ(ToLower("1234,5qweRTY"), "1234,5qwerty");
  EXPECT_EQ(ToLower("qwerty"), "qwerty");
  EXPECT_EQ(ToLower("NOTQWERTY"), "notqwerty");
}

UTEST(TextFormat, Capitalize) {
  using utils::text::Capitalize;
  EXPECT_EQ(Capitalize("1234,5qwerty", "en"), "1234,5qwerty");
  EXPECT_EQ(Capitalize("qwerty", "ar"), "Qwerty");
  EXPECT_EQ(Capitalize("qwerty", "en"), "Qwerty");
  EXPECT_EQ(Capitalize("NOTQWERTY", "en"), "Notqwerty");
}

USERVER_NAMESPACE_END
