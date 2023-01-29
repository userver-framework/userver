#include <userver/utest/utest.hpp>

#include <iostream>
#include <unordered_set>

#include <userver/http/common_headers.hpp>
#include <userver/server/http/header_map.hpp>

#include <server/http/header_map_impl/danger.hpp>

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

TEST(HeaderMap, DefaultHeadersHashDistribution) {
  const server::http::header_map_impl::Danger danger{};
  ASSERT_TRUE(danger.IsGreen());

  const auto h1 = danger.HashKey(http::headers::kServer) % 8;
  const auto h2 = danger.HashKey(http::headers::kXYaTraceId) % 8;
  const auto h3 = danger.HashKey(http::headers::kXYaSpanId) % 8;
  const auto h4 = danger.HashKey(http::headers::kXYaRequestId) % 8;
  const auto h5 = danger.HashKey(http::headers::kHost) % 8;
  const auto h6 = danger.HashKey(http::headers::kUserAgent) % 8;
  const auto h7 = danger.HashKey(http::headers::kAccept) % 8;

  std::unordered_set<std::size_t> st{h1, h2, h3, h4, h5, h6, h7};

  std::cout << h1 << " " << h2 << " " << h3 << " " << h4 << " " << h5 << " "
            << h6 << " " << h7 << std::endl;

  EXPECT_EQ(st.size(), 7);
}

USERVER_NAMESPACE_END
