#include <userver/utest/utest.hpp>

#include <userver/server/http/header_map.hpp>

USERVER_NAMESPACE_BEGIN

TEST(HeaderMap, Works) {
  server::http::HeaderMap map{};

  map.Insert("asd", "we");

  EXPECT_TRUE(map.Find("asd"));
  EXPECT_FALSE(map.Find("dsa"));
}

USERVER_NAMESPACE_END
