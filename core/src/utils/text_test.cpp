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

USERVER_NAMESPACE_END
