#include <userver/utest/utest.hpp>

#include <userver/server/http/header_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

void Check(const server::http::HeaderMap& map) {
  const auto it = map.find("asd");
  it->second += "asd";
}

}  // namespace

TEST(HeaderMap, Works) {
  server::http::HeaderMap map{};

  map.Insert("asd", "we");

  {
    const auto it = map.find("asd");

    ASSERT_NE(it, map.end());
    EXPECT_EQ(it->first, "asd");
    EXPECT_EQ(it->second, "we");

    for (const auto& [k, v] : map) {
      std::cout << k << " " << v << std::endl;
    }
    std::cout << "-------------";

    Check(map);
  }
  {
    const auto it = map.find("asd");
    ASSERT_NE(it, map.end());
    EXPECT_EQ(it->second, "weasd");
  }
  {
    map.Erase("asd");
    EXPECT_EQ(map.find("asd"), map.end());
  }

}

USERVER_NAMESPACE_END
