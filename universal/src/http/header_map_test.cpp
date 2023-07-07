#include <gtest/gtest.h>

#include <array>
#include <unordered_set>

#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/http/header_map.hpp>
#include <userver/utils/rand.hpp>
#include <userver/utils/str_icase.hpp>

#include <userver/internal/http/header_map_tests_helper.hpp>

#include <http/header_map/traits.hpp>

USERVER_NAMESPACE_BEGIN

namespace http::headers {

TEST(HeaderMap, Basic) {
  HeaderMap map{};

  const std::string key{"some_key"};
  const std::string value{"some_value"};

  EXPECT_TRUE(map.empty());
  EXPECT_EQ(map.size(), 0);
  EXPECT_EQ(map.find(key), map.end());

  map.emplace(key, value);
  const auto it = map.find(key);
  ASSERT_EQ(map.size(), 1);
  ASSERT_NE(it, map.end());
  EXPECT_EQ(it->second, value);

  map.erase(key);
  EXPECT_TRUE(map.empty());
  EXPECT_EQ(map.size(), 0);
  EXPECT_EQ(map.find(key), map.end());
}

TEST(HeaderMap, Collision) {
  const auto [first_key,
              second_key] = []() -> std::pair<std::string, std::string> {
    const auto hasher = impl::UnsafeConstexprHasher{};
    for (char f = 'a'; f <= 'z'; ++f) {
      for (char s = f + 1; s <= 'z'; ++s) {
        const auto h1 = hasher(std::string_view{&f, 1}) % 16;
        const auto h2 = hasher(std::string_view{&s, 1}) % 16;

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
  map.emplace(first_key, first_value);
  map.emplace(second_key, second_value);
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

TEST(HeaderMap, CollisionsLongProbeDistance) {
  HeaderMap map{};

  constexpr std::size_t kCollisionLen = 6;
  constexpr std::size_t kCollisionsCount = 1UL << kCollisionLen;

  std::vector<std::pair<std::string, std::string>> collisions_with_value;
  collisions_with_value.reserve(kCollisionsCount);

  std::optional<std::size_t> common_hash{};
  for (std::size_t i = 0; i < kCollisionsCount; ++i) {
    std::string s(kCollisionLen, '[');
    for (std::size_t j = 0; j < kCollisionLen; ++j) {
      if ((i >> j) & 1) {
        s[j] = '{';
      }
    }

    const auto hash = impl::UnsafeConstexprHasher{}(s);
    ASSERT_EQ(common_hash.value_or(hash), hash);
    common_hash.emplace(hash);

    collisions_with_value.emplace_back(
        std::piecewise_construct, std::forward_as_tuple(std::move(s)),
        std::forward_as_tuple(std::to_string(i)));
  }

  for (const auto& [k, v] : collisions_with_value) {
    map.emplace(k, v);
  }
  ASSERT_EQ(map.size(), kCollisionsCount);

  for (const auto& [k, v] : collisions_with_value) {
    const auto it = map.find(k);
    ASSERT_NE(it, map.end());
    ASSERT_EQ(it->second, v);
  }
}

TEST(HeaderMap, InsertDoesntOverwrite) {
  HeaderMap map{};

  const std::string key{"key"};
  const std::string first_value{"1"};
  const std::string second_value{"2"};

  map.emplace(key, first_value);
  map.emplace(key, second_value);

  const auto it = map.find(key);
  ASSERT_NE(it, map.end());
  EXPECT_EQ(it->second, first_value);
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

TEST(HeaderMap, InsertOrAssign) {
  HeaderMap map{};

  const std::string key{"key"};
  const std::string first_value{"1"};
  const std::string second_value{"2"};

  map.insert_or_assign(key, first_value);
  map.insert_or_assign(key, second_value);

  EXPECT_EQ(map[key], second_value);
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
    map.emplace(header.first, header.second);
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

  const std::string key{"KeY"};
  const std::string lowercase_key{"key"};

  map.emplace(key, "some value");

  EXPECT_NE(map.find(key), map.end());
  EXPECT_NE(map.find(lowercase_key), map.end());
}

TEST(HeaderMap, CaseInsensitiveInsert) {
  HeaderMap map{};

  const std::string key{"KeY"};
  const std::string lowercase_key{"key"};

  map.emplace(key, "1");
  map.emplace(lowercase_key, "2");

  EXPECT_EQ(map.size(), 1);
  const auto it = map.find(key);
  ASSERT_NE(it, map.end());
  EXPECT_TRUE(utils::StrIcaseEqual{}(it->first, lowercase_key));
  EXPECT_EQ(it->second, "1");
}

TEST(HeaderMap, CaseInsensitiveInsertAppend) {
  HeaderMap map{};

  const std::string key{"KeY"};
  const std::string lowercase_key{"key"};

  map.InsertOrAppend(key, "1");
  map.InsertOrAppend(lowercase_key, "2");

  EXPECT_EQ(map.size(), 1);
  const auto it = map.find(key);
  ASSERT_NE(it, map.end());
  EXPECT_TRUE(utils::StrIcaseEqual{}(it->first, lowercase_key));
  EXPECT_EQ(it->second, "1,2");
}

TEST(HeaderMap, Iteration) {
  const std::array<std::pair<std::string, std::string>, 3> headers{
      std::pair<std::string, std::string>{"a", "1"},
      std::pair<std::string, std::string>{"b", "2"},
      std::pair<std::string, std::string>{"c", "3"}};

  HeaderMap map{};
  for (const auto& [k, v] : headers) {
    map.emplace(k, v);
  }

  ASSERT_EQ(map.size(), 3);

  std::size_t index = 0;
  for (const auto& [k, v] : map) {
    ASSERT_LT(index, headers.size());
    const auto r_index = headers.size() - 1 - index;
    EXPECT_EQ(headers[r_index].first, k);
    EXPECT_EQ(headers[r_index].second, v);

    ++index;
  }
}

TEST(HeaderMap, EraseFromEmpty) {
  HeaderMap map{};
  map.erase(std::string_view{"some_key"});
}

TEST(HeaderMap, Count) {
  HeaderMap map{};

  const std::string key{"some_key"};

  EXPECT_EQ(map.count(key), 0);

  map.emplace(key, "some_value");
  EXPECT_EQ(map.count(key), 1);

  map.erase(key);
  EXPECT_EQ(map.count(key), 0);
}

TEST(HeaderMap, Emplace) {
  HeaderMap map{};

  const std::string key{"key"};
  constexpr const char* data{"some_value"};
  constexpr std::size_t data_size = 4;

  map.emplace(key, data, data_size);

  const auto it = map.find(key);
  ASSERT_NE(it, map.end());

  EXPECT_EQ(it->second, (std::string{data, data_size}));
}

TEST(HeaderMap, SubscriptOperator) {
  HeaderMap map{};

  const std::string existing_key{"some_key"};
  const std::string existing_value{"some_value"};

  map.emplace(existing_key, existing_value);

  const auto& value = map[existing_key];
  EXPECT_EQ(value, existing_value);

  const std::string other_key{"some_other_key"};
  const std::string other_value{"some_other_value"};
  EXPECT_TRUE(map[other_key].empty());

  map[other_key] = other_value;
  const auto it = map.find(other_key);
  ASSERT_NE(it, map.end());
  EXPECT_EQ(it->second, other_value);
}

TEST(HeaderMap, IteratorConversion) {
  const std::pair<std::string, std::string> first_kvp{"a", "a"};
  const std::pair<std::string, std::string> second_kvp{"b", "b"};
  const HeaderMap map{first_kvp, second_kvp};

  std::vector<std::pair<std::string, std::string>> vec{};
  vec.reserve(map.size());

  std::copy(map.begin(), map.end(), std::back_inserter(vec));

  EXPECT_EQ(vec.size(), 2);
  if (vec[0].first > vec[1].first) {
    std::swap(vec[0], vec[1]);
  }

  EXPECT_EQ(vec[0], first_kvp);
  EXPECT_EQ(vec[1], second_kvp);
}

TEST(HeaderMap, InsertFromIterators) {
  const std::vector<std::pair<std::string, std::string>> vec{{"a", "a"},
                                                             {"b", "b"}};

  const auto validate = [](const HeaderMap& map) {
    EXPECT_EQ(map.size(), 2);
    EXPECT_EQ(map.count(std::string_view{"a"}), 1);
    EXPECT_EQ(map.count(std::string_view{"b"}), 1);
  };

  {
    const HeaderMap map{vec.begin(), vec.end()};
    validate(map);
  }

  {
    HeaderMap map{};
    map.insert(vec.begin(), vec.end());
    validate(map);
  }
}

TEST(HeaderMap, ParseFromSerializeToJson) {
  const std::string first_key{"first_key"};
  const std::string first_value{"first_value"};

  const std::string second_key{"second_key"};
  const std::string second_value{"second_value"};

  const auto json = [&] {
    formats::json::ValueBuilder builder{};

    builder[first_key] = first_value;
    builder[second_key] = second_value;

    return builder.ExtractValue();
  }();

  const auto map = json.As<HeaderMap>();

  const auto validate_key = [&map](const std::string& key,
                                   const std::string& value) {
    const auto it = map.find(key);
    ASSERT_NE(it, map.end());
    EXPECT_EQ(it->second, value);
  };
  validate_key(first_key, first_value);
  validate_key(second_key, second_value);

  const auto serialized_to_json =
      formats::json::ValueBuilder{map}.ExtractValue();

  EXPECT_EQ(serialized_to_json, json);
}

TEST(HeaderMap, TooManyHeaders) {
  constexpr std::size_t max_headers_count =
      http::headers::header_map::Traits::kMaxSize;
  HeaderMap map{};
  map.reserve(max_headers_count);

  const auto insert_too_many_headers = [&map] {
    for (std::size_t i = 0; i < max_headers_count / 4 * 3 + 1; ++i) {
      map.emplace(std::to_string(i), "value");
    }
  };

  EXPECT_THROW(insert_too_many_headers(), HeaderMap::TooManyHeadersException);
}

TEST(HeaderMap, IterateAndErase) {
  // some random big enough number
  constexpr std::size_t headers_count = 137;

  HeaderMap map{};
  for (std::size_t i = 0; i < headers_count; ++i) {
    map.emplace(std::to_string(i), "1");
  }

  std::size_t cnt_erased = 0;
  for (auto it = map.begin(); it != map.end();) {
    it = map.erase(it);
    ++cnt_erased;
  }

  EXPECT_TRUE(map.empty());
  EXPECT_EQ(cnt_erased, headers_count);
}

TEST(HeaderMap, ClearResetsToGreen) {
  HeaderMap map{};

  map[USERVER_NAMESPACE::http::headers::kHost] = "host";
  map[USERVER_NAMESPACE::http::headers::kConnection] = "connection";

  EXPECT_FALSE(TestsHelper::GetMapDanger(map).IsRed());

  TestsHelper::ForceIntoRedState(map);
  EXPECT_TRUE(TestsHelper::GetMapDanger(map).IsRed());

  EXPECT_EQ(map[USERVER_NAMESPACE::http::headers::kHost], "host");
  EXPECT_EQ(map[USERVER_NAMESPACE::http::headers::kConnection], "connection");

  map.clear();
  EXPECT_EQ(map.find(USERVER_NAMESPACE::http::headers::kHost), map.end());
  EXPECT_EQ(map.find(USERVER_NAMESPACE::http::headers::kConnection), map.end());
  EXPECT_FALSE(TestsHelper::GetMapDanger(map).IsRed());
}

TEST(HeaderMap, OperationsOnRedState) {
  HeaderMap map{};

  TestsHelper::ForceIntoRedState(map);
  ASSERT_TRUE(TestsHelper::GetMapDanger(map).IsRed());

  const auto first_header = USERVER_NAMESPACE::http::headers::kHost;
  const std::string first_value = "localhost";

  const std::string second_header = "X-Some-Header";
  const std::string second_value = "some_value";

  map[first_header] = first_value;
  map[second_header] = second_value;

  ASSERT_TRUE(TestsHelper::GetMapDanger(map).IsRed());
  EXPECT_EQ(map[first_header], first_value);
  EXPECT_EQ(map[second_header], second_value);
  EXPECT_EQ(map.size(), 2);
}

namespace {

std::vector<std::string> GenerateCollisions(std::size_t count) {
  std::vector<std::string> result;
  result.reserve(count);

  std::size_t len = 1;
  for (; (1UL << len) < count; ++len)
    ;

  for (std::size_t i = 0; i < count; ++i) {
    std::string s(len, '[');
    for (std::size_t j = 0; j < len; ++j) {
      if ((i >> j) & 1) {
        s[j] = '{';
      }
    }

    result.push_back(std::move(s));
  }

  std::optional<std::size_t> common_hash{};
  for (const auto& s : result) {
    const auto h = impl::UnsafeConstexprHasher{}(s);
    if (common_hash.value_or(h) != h) {
      throw std::runtime_error{"Collisions generation is broken"};
    }
    common_hash.emplace(h);
  }

  return result;
}

TEST(HeaderMap, EraseWithCollisions) {
  const auto collisions = GenerateCollisions(100);

  const auto original_map = [&collisions] {
    HeaderMap map{};
    for (const auto& s : collisions) {
      map.emplace(s, "1");
    }
    return map;
  }();
  ASSERT_EQ(TestsHelper::GetMapMaxDisplacement(original_map) + 1,
            collisions.size());

  const auto validate_erase =
      [](HeaderMap map, const std::vector<std::string>& shuffled_content) {
        ASSERT_EQ(map.size(), shuffled_content.size());

        std::size_t expected_size = map.size();
        for (const auto& s : shuffled_content) {
          map.erase(s);
          --expected_size;

          ASSERT_EQ(map.size(), expected_size);
        }
      };

  validate_erase(original_map, collisions);
  {
    auto reserved = collisions;
    std::reverse(reserved.begin(), reserved.end());
    validate_erase(original_map, collisions);
  }
  {
    auto random_shuffle = collisions;
    std::shuffle(random_shuffle.begin(), random_shuffle.end(),
                 utils::DefaultRandom());
    validate_erase(original_map, random_shuffle);
  }
  {
    auto map_copy = original_map;
    auto random_shuffle = collisions;
    std::shuffle(random_shuffle.begin(), random_shuffle.end(),
                 utils::DefaultRandom());

    for (std::size_t i = 0; i < random_shuffle.size() / 2; ++i) {
      map_copy.erase(random_shuffle[i]);
    }
    ASSERT_EQ(map_copy.size(),
              random_shuffle.size() - (random_shuffle.size() / 2));
    for (std::size_t i = random_shuffle.size() / 2; i < random_shuffle.size();
         ++i) {
      const auto it = map_copy.find(random_shuffle[i]);
      ASSERT_NE(it, map_copy.end());
      ASSERT_EQ(it->first, random_shuffle[i]);
    }
  }
}

}  // namespace

namespace {
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
    USERVER_NAMESPACE::http::headers::kXRequestId,
    USERVER_NAMESPACE::http::headers::kXBackendServer,
    USERVER_NAMESPACE::http::headers::kXTaxiEnvoyProxyDstVhost,
    USERVER_NAMESPACE::http::headers::kDate,
    USERVER_NAMESPACE::http::headers::kConnection,
    USERVER_NAMESPACE::http::headers::kCookie};
static_assert(std::size(kDefaultHeaders) == 16);

constexpr std::string_view kAllUsedHeaders[] = {
    USERVER_NAMESPACE::http::headers::kContentType,
    USERVER_NAMESPACE::http::headers::kContentEncoding,
    USERVER_NAMESPACE::http::headers::kContentLength,
    USERVER_NAMESPACE::http::headers::kTransferEncoding,
    USERVER_NAMESPACE::http::headers::kHost,
    USERVER_NAMESPACE::http::headers::kAccept,
    USERVER_NAMESPACE::http::headers::kAcceptEncoding,
    USERVER_NAMESPACE::http::headers::kAcceptLanguage,
    USERVER_NAMESPACE::http::headers::kApiKey,
    USERVER_NAMESPACE::http::headers::kUserAgent,
    USERVER_NAMESPACE::http::headers::kXRequestApplication,
    USERVER_NAMESPACE::http::headers::kDate,
    USERVER_NAMESPACE::http::headers::kWarning,
    USERVER_NAMESPACE::http::headers::kAccessControlAllowHeaders,
    USERVER_NAMESPACE::http::headers::kAllow,
    USERVER_NAMESPACE::http::headers::kServer,
    USERVER_NAMESPACE::http::headers::kSetCookie,
    USERVER_NAMESPACE::http::headers::kConnection,
    USERVER_NAMESPACE::http::headers::kCookie,
    USERVER_NAMESPACE::http::headers::kXYaRequestId,
    USERVER_NAMESPACE::http::headers::kXYaTraceId,
    USERVER_NAMESPACE::http::headers::kXYaSpanId,
    USERVER_NAMESPACE::http::headers::kXRequestId,
    USERVER_NAMESPACE::http::headers::kXBackendServer,
    USERVER_NAMESPACE::http::headers::kXTaxiEnvoyProxyDstVhost,
    USERVER_NAMESPACE::http::headers::kXBaggage,
    USERVER_NAMESPACE::http::headers::kXYaTaxiAllowAuthRequest,
    USERVER_NAMESPACE::http::headers::kXYaTaxiAllowAuthResponse,
    USERVER_NAMESPACE::http::headers::kXYaTaxiServerHostname,
    USERVER_NAMESPACE::http::headers::kXYaTaxiClientTimeoutMs,
    USERVER_NAMESPACE::http::headers::kXYaTaxiRatelimitedBy,
    USERVER_NAMESPACE::http::headers::kXYaTaxiRatelimitReason,
};

template <std::size_t Size, typename HeadersArray>
bool CheckNoCollisions(const HeadersArray& headers,
                       const impl::UnsafeConstexprHasher& hasher) {
  static_assert((Size & (Size - 1)) == 0);

  std::array<std::uint8_t, Size> cnt{0};
  for (const auto sw : headers) {
    cnt[hasher(sw) & (Size - 1)] = 1;
  }

  std::size_t occupied = 0;
  for (const auto c : cnt) {
    occupied += c;
  }

  return occupied == std::size(headers);
}

}  // namespace

TEST(HeaderMap, DefaultHeadersHashDistribution) {
  const auto check_uniqueness = [](const auto& headers) {
    const std::unordered_set<std::string_view> st(std::begin(headers),
                                                  std::end(headers));
    ASSERT_EQ(st.size(), std::size(headers));
  };

  check_uniqueness(kDefaultHeaders);
  check_uniqueness(kAllUsedHeaders);

  const impl::UnsafeConstexprHasher hasher{};

  static_assert(32 >= std::size(kDefaultHeaders));
  ASSERT_TRUE(CheckNoCollisions<32>(kDefaultHeaders, hasher));

  static_assert(64 >= std::size(kAllUsedHeaders));
  ASSERT_TRUE(CheckNoCollisions<64>(kAllUsedHeaders, hasher));
}

}  // namespace http::headers

USERVER_NAMESPACE_END
