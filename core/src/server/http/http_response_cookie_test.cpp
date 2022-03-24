#include <userver/utest/utest.hpp>

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
  EXPECT_EQ(cookie.ToString(),
            "name1=value1; Domain=domain.com; Path=/; Expires=Wed, 12 Jun 2019 "
            "16:51:45 GMT; "
            "Max-Age=3600; Secure; HttpOnly");
  EXPECT_EQ(cookie.Name(), "name1");
  EXPECT_EQ(cookie.Value(), "value1");
  EXPECT_TRUE(cookie.IsSecure());
  EXPECT_TRUE(cookie.IsHttpOnly());
  EXPECT_FALSE(cookie.IsPermanent());
  EXPECT_EQ(cookie.Path(), "/");
  EXPECT_EQ(cookie.Domain(), "domain.com");
  EXPECT_EQ(cookie.MaxAge().count(), 3600);
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
  std::vector<std::pair<std::string, std::string>> goods = {
      {"name", "value"},                               //
      {"a+b", "value"},                                //
      {"name", ""},                                    //
      {"name", "\"\""},                                //
      {"name", "\"value\""},                           //
      {"name", "!#$%&'()*+-./09:<=>?@AZ[]^_`az{|}~"},  //
      {"na%20%me%", "%20%"},                           //
  };

  std::vector<std::pair<std::string, std::string>> bads = {
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
      {"name", "\" \""},        //
      {"name", "\"val ue\""},   //
      {"name", "\"val\"ue\""},  //
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

USERVER_NAMESPACE_END
