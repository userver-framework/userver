#include <userver/utest/utest.hpp>

#include <iostream>
#include <unordered_set>

#include <userver/http/common_headers.hpp>
#include <userver/server/http/header_map.hpp>

#include <server/http/header_map_impl/danger.hpp>

USERVER_NAMESPACE_BEGIN

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

    it->second += "asd";
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

constexpr std::string_view kDefaultHeaders[] = {
    http::headers::kHost,
    http::headers::kUserAgent,
    http::headers::kAccept,
    http::headers::kAcceptEncoding,
    http::headers::kXYaSpanId,
    http::headers::kXYaTraceId,
    http::headers::kXYaRequestId,
    http::headers::kXYaTaxiClientTimeoutMs,
    http::headers::kContentLength,
    http::headers::kContentType,
    "cookie",
    http::headers::kXRequestId,
    http::headers::kXBackendServer,
    http::headers::kXTaxiEnvoyProxyDstVhost,
    http::headers::kDate,
    http::headers::kConnection};
static_assert(std::size(kDefaultHeaders) <= 16);

TEST(HeaderMap, DefaultHeadersHashDistribution) {
  std::unordered_set<std::string_view> st(std::begin(kDefaultHeaders),
                                          std::end(kDefaultHeaders));
  EXPECT_EQ(st.size(), std::size(kDefaultHeaders));

  for (const auto sw : kDefaultHeaders) {
    ASSERT_TRUE(server::http::header_map_impl::IsLowerCase(sw));
  }

  {
    std::array<bool, 16> cnt{};
    auto hasher = server::http::header_map_impl::RuntimeHasher{};
    //hasher.SetSeed(seed);
    for (const auto sw : kDefaultHeaders) {
      cnt[hasher(sw.data(), sw.size()) % 16] = true;
    }

    bool ok = true;
    for (const auto has : cnt) {
      ok &= has;
    }

    EXPECT_TRUE(ok);
  }
}

USERVER_NAMESPACE_END
