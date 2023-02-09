#include <userver/utest/utest.hpp>

#include <sys/param.h>

#include <userver/server/http/http_response_cookie.hpp>

USERVER_NAMESPACE_BEGIN

TEST(HttpCookie, Simple) {
  server::http::Cookie cookie{"name1", "value1"};
  EXPECT_EQ(cookie.ToString(), "name1=value1");
  cookie.SetSecure().SetHttpOnly();
  EXPECT_EQ(cookie.ToString(), "name1=value1; Secure; HttpOnly");
  cookie.SetMaxAge(std::chrono::seconds{3600});
  EXPECT_EQ(cookie.ToString(), "name1=value1; Max-Age=3600; Secure; HttpOnly");
  cookie.SetPath("/");
  EXPECT_EQ(cookie.ToString(),
            "name1=value1; Path=/; Max-Age=3600; Secure; HttpOnly");
  cookie.SetDomain("domain.com");
  EXPECT_EQ(cookie.ToString(),
            "name1=value1; Domain=domain.com; Path=/; Max-Age=3600; Secure; "
            "HttpOnly");
  cookie.SetExpires(
      std::chrono::system_clock::time_point{std::chrono::seconds{1560358305}});

  const auto* const expected =
      "name1=value1; Domain=domain.com; Path=/; Expires=Wed, 12 Jun 2019 "
      "16:51:45 "
#if defined(BSD) && !defined(__APPLE__)
      "UTC; "
#else
      "GMT; "
#endif
      "Max-Age=3600; Secure; HttpOnly";
  EXPECT_EQ(cookie.ToString(), expected);

  cookie.SetSameSite("None");
  const auto* const expected2 =
      "name1=value1; Domain=domain.com; Path=/; Expires=Wed, 12 Jun 2019 "
      "16:51:45 "
#if defined(BSD) && !defined(__APPLE__)
      "UTC; "
#else
      "GMT; "
#endif
      "Max-Age=3600; Secure; SameSite=None; HttpOnly";
  EXPECT_EQ(cookie.ToString(), expected2);

  EXPECT_EQ(cookie.Name(), "name1");
  EXPECT_EQ(cookie.Value(), "value1");
  EXPECT_TRUE(cookie.IsSecure());
  EXPECT_TRUE(cookie.IsHttpOnly());
  EXPECT_FALSE(cookie.IsPermanent());
  EXPECT_EQ(cookie.Path(), "/");
  EXPECT_EQ(cookie.Domain(), "domain.com");
  EXPECT_EQ(cookie.MaxAge().count(), 3600);
  EXPECT_EQ(cookie.SameSite(), "None");
  EXPECT_EQ(cookie.Expires().time_since_epoch().count() *
                std::chrono::system_clock::period::num /
                std::chrono::system_clock::period::den,
            1560358305L);
  cookie.SetPermanent();
  EXPECT_TRUE(cookie.IsPermanent());
  EXPECT_GT(cookie.Expires().time_since_epoch().count() *
                std::chrono::system_clock::period::num /
                std::chrono::system_clock::period::den,
            1560358305L);
}

TEST(HttpCookie, Validation) {
  const std::vector<std::pair<std::string, std::string>> goods = {
      {"name", "value"},                               //
      {"a+b", "value"},                                //
      {"name", ""},                                    //
      {"name", "\"\""},                                //
      {"name", "\"value\""},                           //
      {"name", "!#$%&'()*+-./09:<=>?@AZ[]^_`az{|}~"},  //
      {"na%20%me%", "%20%"},                           //
  };

  const std::vector<std::pair<std::string, std::string>> bads = {
      {"", "value"},      //
      {"a=b", "value"},   //
      {"a b", "value"},   //
      {"a\tb", "value"},  //
      {"a/b", "value"},   //
      {"a;b", "value"},   //
      {"a@b", "value"},   //

      {"name", "\""},           //
      {"name", "a\"b"},         //
      {"name", "\"a"},          //
      {"name", "a\""},          //
      {"name", " "},            //
      {"name", R"(" ")"},       //
      {"name", R"("val ue")"},  //
      {"name", R"("val"ue")"},  //
      {"name", ","},            //
      {"name", ";"},            //
      {"name", "\\"},           //

      {"name", "\x80"},       //
      {"na\x80me", "value"},  //
      {"name", "val\xFFue"},  //
      {"na\xFFme", "value"},  //
      {"name", "val\xABue"},  //
      {"na\xABme", "value"},  //
      {"name", "валуе"},      //
      {"наме", "value"},      //
  };

  auto create_cookie = [](const std::pair<std::string, std::string>& kv) {
    return server::http::Cookie{kv.first, kv.second};
  };

  for (const auto& good : goods) {
    UEXPECT_NO_THROW(create_cookie(good));
  }

  for (const auto& bad : bads) {
    UEXPECT_THROW(create_cookie(bad), std::runtime_error);
  }
}

TEST(HttpCookie, FromString) {
  const std::vector<std::string> good_cookies_as_str = {
      // NOLINTNEXTLINE(bugprone-suspicious-missing-comma)
      "name1=value1; Domain=domain.com; Path=/; Expires=Wed, 12 Jun 2019 "
      "16:51:45 GMT; Max-Age=3600; Secure; SameSite=None; HttpOnly",
      "name1=value1; Domain=domain.com; Path=/; Expires=Wed, 12 Jun 2019 "
      "16:51:45 GMT; Max-Age=0; Secure; SameSite=None; HttpOnly",
      "name1=value1; Domain=domain.com; Path=/my_beautiful_path; "
      "Expires=Thu, 01 Jan 1970 00:00:00 GMT; Max-Age=1800; Secure; "
      "SameSite=None; HttpOnly",
      "name1=; Domain=domain.com; Path=/my_beautiful_path; "
      "Expires=Thu, 01 Jan 1970 00:00:00 GMT; Max-Age=1800; Secure; "
      "SameSite=None; HttpOnly",
      "name=",
      "-|0|||-=",
  };
  for (const auto& cookie_as_str : good_cookies_as_str) {
    auto cookie = server::http::Cookie::FromString(cookie_as_str);
    EXPECT_EQ(cookie.value().ToString(), cookie_as_str);
  }

  const utils::StrIcaseEqual equal;
  const std::vector<std::string> cookies_as_str_icase = {
      "name1=value1; Domain=domain.com; Path=/; Expires=Wed, 12 Jun 2019 "
      "16:51:45 GMT; Max-Age=3600; Secure; SameSite=None; HttpOnly",
      "name1=value1; Domain=domain.com; Path=/; Expires=Wed, 12 Jun 2019 "
      "16:51:45 GMT; Max-Age=0; secure; SameSItE=None; HttpOnly",
      "name1=value1; Domain=domain.com; path=/my_beautiful_path; "
      "expires=Thu, 01 Jan 1970 00:00:00 GMT; mAx-Age=1800; Secure; "
      "SameSite=None; httponly",
      "name1=; domain=domain.com; path=/my_beautiful_path; "
      "expires=Thu, 01 Jan 1970 00:00:00 GMT; max-age=1800; secure; "
      "samesite=None; httponly",
  };
  for (const auto& cookie_as_str : cookies_as_str_icase) {
    auto cookie = server::http::Cookie::FromString(cookie_as_str);
    EXPECT_TRUE(equal(cookie.value().ToString(), cookie_as_str));
  }

  const std::vector<std::string> bad_cookies_as_str = {
      // NOLINTNEXTLINE(bugprone-suspicious-missing-comma)
      "name1=value1; Domain=domain.com; Path=; Expires=Wed, 12 Jun 2019 "
      "16:51:45 GMT; Max-Age=0; Secure; SameSite=None; HttpOnly",
      "name1=value1; Domain=domain.com; Path=/my_beautiful_path; "
      "Expires=Thu, 01 Jan 1969 00:00:00 GMT; Max-Age=1800; Secure; "
      "SameSite=None; HttpOnly",
      "name1=; Domain; Path=/my_beautiful_path; "
      "Expires=Thu, 01 Jan 1970 00:00:00 GMT; Max-Age=1800; Secure; "
      "SameSite=None; HttpOnly",
      "name1=; Domain=domain.com; Path=/my_beautiful_path; ",
      "name;Path=/",
      "name; Path=/",
      " -|0|||-=",
      "name",
      "name=;",
      "name=1;Domain=;",
      "name=;Domain=;",
      "name=1;Domain=",
      "name=1; Domain="
      "name=;Domain=domain.com",
  };
  for (const auto& cookie_as_str : bad_cookies_as_str) {
    auto cookie = server::http::Cookie::FromString(cookie_as_str);
    EXPECT_FALSE(equal(cookie.value().ToString(), cookie_as_str));
  }
}

USERVER_NAMESPACE_END
