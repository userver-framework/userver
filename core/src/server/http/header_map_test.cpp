#include <userver/utest/utest.hpp>

#include <iostream>
#include <unordered_set>

#include <userver/http/common_headers.hpp>
#include <userver/server/http/header_map.hpp>

#include <server/http/header_map_impl/danger.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

TEST(HeaderMap, Basic) {
  HeaderMap map{};

  const std::string key{"some_key"};
  const std::string value{"some_value"};

  EXPECT_TRUE(map.empty());
  EXPECT_EQ(map.size(), 0);
  EXPECT_EQ(map.find(key), map.end());

  map.Insert(key, value);
  const auto it = map.find(key);
  ASSERT_EQ(map.size(), 1);
  ASSERT_NE(it, map.end());
  EXPECT_EQ(it->second, value);

  map.Erase(key);
  EXPECT_TRUE(map.empty());
  EXPECT_EQ(map.size(), 0);
  EXPECT_EQ(map.find(key), map.end());
}

TEST(HeaderMap, Collision) {
  const auto [first_key,
              second_key] = []() -> std::pair<std::string, std::string> {
    const auto hasher = header_map_impl::RuntimeHasher{};
    for (char f = 'a'; f <= 'z'; ++f) {
      for (char s = f + 1; s <= 'z'; ++s) {
        const auto h1 = hasher(&f, 1) % 16;
        const auto h2 = hasher(&s, 1) % 16;

        if (h1 == h2) {
          return {{f}, {s}};
        }
      }
    }

    return {};
  }();

  ASSERT_FALSE(first_key.empty());
  ASSERT_FALSE(second_key.empty());
  ASSERT_NE(first_key, second_key);

  const std::string first_value{"1"};
  const std::string second_value{"2"};

  HeaderMap map{};
  map.Insert(first_key, first_value);
  map.Insert(second_key, second_value);
  EXPECT_EQ(map.size(), 2);

  {
    const auto first_it = map.find(first_key);
    ASSERT_NE(first_it, map.end());
    EXPECT_EQ(first_it->first, first_key);
    EXPECT_EQ(first_it->second, first_value);
  }

  {
    const auto second_it = map.find(second_key);
    ASSERT_NE(second_it, map.end());
    EXPECT_EQ(second_it->first, second_key);
    EXPECT_EQ(second_it->second, second_value);
  }
}

TEST(HeaderMap, InsertOverwrite) {
  HeaderMap map{};

  const std::string key{"key"};
  const std::string first_value{"1"};
  const std::string second_value{"2"};

  map.Insert(key, first_value);
  map.Insert(key, second_value);

  const auto it = map.find(key);
  ASSERT_NE(it, map.end());
  EXPECT_EQ(it->second, second_value);
}

TEST(HeaderMap, InsertAppend) {
  HeaderMap map{};

  const std::string key{"key"};
  const std::string first_value{"1"};
  const std::string second_value{"2"};

  map.InsertOrAppend(key, first_value);
  map.InsertOrAppend(key, second_value);

  EXPECT_EQ(map.size(), 1);
  const auto it = map.find(key);
  ASSERT_NE(it, map.end());
  EXPECT_EQ(it->second, first_value + "," + second_value);
}

TEST(HeaderMap, InsertAppendNew) {
  HeaderMap map{};

  const std::string key{"key"};
  const std::string value{"value"};

  map.InsertOrAppend(key, value);

  const auto it = map.find(key);
  ASSERT_NE(it, map.end());
  EXPECT_EQ(it->second, value);
}

TEST(HeaderMap, Resize) {
  HeaderMap map{};

  std::array<std::pair<std::string, std::string>, 20> headers{};
  char index = 0;
  for (auto& header : headers) {
    header.first.push_back('a' + index);
    header.second.push_back('a' + index);

    ++index;
  }

  for (const auto& header : headers) {
    map.Insert(header.first, header.second);
  }

  EXPECT_EQ(map.size(), headers.size());
  for (auto& header : headers) {
    const auto it = map.find(header.first);
    ASSERT_NE(it, map.end());
    ASSERT_EQ(it->second, header.second);
  }
}

TEST(HeaderMap, CaseInsensitiveFind) {
  HeaderMap map{};

  const std::string key{"Key"};
  const std::string lowercase_key{"key"};

  map.Insert(key, "some value");

  EXPECT_NE(map.find(key), map.end());
  EXPECT_NE(map.find(lowercase_key), map.end());
}

TEST(HeaderMap, CaseInsensitiveInsert) {
  HeaderMap map{};

  const std::string key{"Key"};
  const std::string lowercase_key{"key"};

  map.Insert(key, "1");
  map.Insert(lowercase_key, "2");

  EXPECT_EQ(map.size(), 1);
  const auto it = map.find(key);
  ASSERT_NE(it, map.end());
  EXPECT_EQ(it->first, lowercase_key);
  EXPECT_EQ(it->second, "2");
}

TEST(HeaderMap, CaseInsensitiveInsertAppend) {
  HeaderMap map{};

  const std::string key{"Key"};
  const std::string lowercase_key{"key"};

  map.InsertOrAppend(key, "1");
  map.InsertOrAppend(lowercase_key, "2");

  EXPECT_EQ(map.size(), 1);
  const auto it = map.find(key);
  ASSERT_NE(it, map.end());
  EXPECT_EQ(it->first, lowercase_key);
  EXPECT_EQ(it->second, "1,2");
}

TEST(HeaderMap, Iteration) {
  const std::array<std::pair<std::string, std::string>, 3> headers{
      std::pair<std::string, std::string>{"a", "1"},
      std::pair<std::string, std::string>{"b", "2"},
      std::pair<std::string, std::string>{"c", "3"}
  };

  HeaderMap map{};
  for (const auto& [k, v] : headers) {
    map.Insert(k, v);
  }

  ASSERT_EQ(map.size(), 3);

  std::size_t index = 0;
  for (const auto& [k, v] : map) {
    ASSERT_LT(index, headers.size());
    EXPECT_EQ(headers[index].first, k);
    EXPECT_EQ(headers[index].second, v);

    ++index;
  }
}

constexpr std::string_view kDefaultHeaders[] = {
    USERVER_NAMESPACE::http::headers::kHost,
    USERVER_NAMESPACE::http::headers::kUserAgent,
    USERVER_NAMESPACE::http::headers::kAccept,
    USERVER_NAMESPACE::http::headers::kAcceptEncoding,
    USERVER_NAMESPACE::http::headers::kXYaSpanId,
    USERVER_NAMESPACE::http::headers::kXYaTraceId,
    USERVER_NAMESPACE::http::headers::kXYaRequestId,
    USERVER_NAMESPACE::http::headers::kXYaTaxiClientTimeoutMs,
    USERVER_NAMESPACE::http::headers::kContentLength,
    USERVER_NAMESPACE::http::headers::kContentType,
    "cookie",
    USERVER_NAMESPACE::http::headers::kXRequestId,
    USERVER_NAMESPACE::http::headers::kXBackendServer,
    USERVER_NAMESPACE::http::headers::kXTaxiEnvoyProxyDstVhost,
    USERVER_NAMESPACE::http::headers::kDate,
    USERVER_NAMESPACE::http::headers::kConnection};
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
    const auto hasher = server::http::header_map_impl::RuntimeHasher{};
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

}  // namespace server::http

USERVER_NAMESPACE_END
