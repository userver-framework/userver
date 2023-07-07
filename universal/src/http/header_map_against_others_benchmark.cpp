// We don't have neither abseil nor 1.82+ boost in CI/default environment,
// but still want to be able to run these locally:
// for that we define USERVER_HEADER_MAP_AGAINST_OTHERS_BENCHMARK with cmake.
#ifdef HEADER_MAP_AGAINST_OTHERS_BENCHMARK

#include <benchmark/benchmark.h>

#include <unordered_map>
#include <vector>

#include <absl/container/flat_hash_map.h>
#include <boost/unordered/unordered_flat_map.hpp>

#include <userver/http/common_headers.hpp>
#include <userver/http/header_map.hpp>
#include <userver/utils/str_icase.hpp>

#include <utils/gbench_auxilary.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

struct TransparentICaseHash final {
  using is_transparent = void;
  std::size_t operator()(std::string_view str) const noexcept {
    return hasher_(str);
  }

  utils::StrIcaseHash hasher_{};
};

struct TransparentICaseEqual final {
  using is_transparent = void;
  bool operator()(std::string_view lhs, std::string_view rhs) const noexcept {
    return utils::StrIcaseEqual{}(lhs, rhs);
  }
};

template <typename KeyEq>
class LinearSearch final {
 public:
  void reserve(std::size_t capacity) { values_.reserve(capacity); }

  auto find(std::string_view key) {
    return std::find_if(
        values_.begin(), values_.end(),
        [this, key](const auto& kvp) { return cmp_(key, kvp.first); });
  }

  auto try_emplace(std::string key, std::string value) {
    auto it = find(key);
    auto inserted = false;

    if (it == values_.end()) {
      values_.emplace_back(std::piecewise_construct,
                           std::forward_as_tuple(std::move(key)),
                           std::forward_as_tuple(std::move(value)));
      it = --values_.end();
      inserted = true;
    }

    return std::make_pair(it, inserted);
  }

  void erase(std::string_view key) {
    for (std::size_t i = 0; i < values_.size(); ++i) {
      if (cmp_(key, values_[i].first)) {
        if (i + 1 != values_.size()) {
          std::swap(values_[i], values_.back());
        }
        values_.pop_back();

        return;
      }
    }
  }

 private:
  KeyEq cmp_{};
  std::vector<std::pair<std::string, std::string>> values_;
};

using HeaderMap = http::headers::HeaderMap;
using AbslMap =
    absl::flat_hash_map<std::string, std::string, TransparentICaseHash,
                        TransparentICaseEqual>;
using BoostFlatHashMap =
    boost::unordered_flat_map<std::string, std::string, TransparentICaseHash,
                              TransparentICaseEqual>;
using LinearSearchMap = LinearSearch<TransparentICaseEqual>;
using StdUnorderedMap =
    std::unordered_map<std::string, std::string, TransparentICaseHash,
                       TransparentICaseEqual>;

template <typename Map>
class MapProxyBase {
 public:
  MapProxyBase(Map& map) : map_{map} {}
  ~MapProxyBase() = default;

  void Reserve(std::size_t capacity) { map_.reserve(capacity); }

  auto Find(std::string_view header) { return Get().find(header); }

  auto InsertOrAppend(std::string key, std::string value) {
    auto [it, inserted] = Get().try_emplace(std::move(key), std::move(value));
    if (!inserted) {
      it->second += ',';
      it->second += value;
    }

    return it;
  }

  auto FindPredefined(const http::headers::PredefinedHeader& header) {
    return Get().find(header);
  }

  void Erase(const http::headers::PredefinedHeader& header) {
    Get().erase(header);
  }

 protected:
  Map& Get() { return map_; }

 private:
  Map& map_;
};

template <typename Map>
class MapProxy;

template <>
class MapProxy<HeaderMap> final : public MapProxyBase<HeaderMap> {
 public:
  using MapProxyBase<HeaderMap>::MapProxyBase;

  // NOLINTNEXTLINE
  auto InsertOrAppend(std::string key, std::string value) {
    return Get().InsertOrAppend(std::move(key), std::move(value));
  }

  // NOLINTNEXTLINE
  auto FindPredefined(const http::headers::PredefinedHeader& header) {
    return Get().find(header);
  }
};

template <>
class MapProxy<AbslMap> final : public MapProxyBase<AbslMap> {
 public:
  using MapProxyBase<AbslMap>::MapProxyBase;
};

template <>
class MapProxy<BoostFlatHashMap> final : public MapProxyBase<BoostFlatHashMap> {
 public:
  using MapProxyBase<BoostFlatHashMap>::MapProxyBase;
};

template <>
class MapProxy<LinearSearchMap> final : public MapProxyBase<LinearSearchMap> {
 public:
  using MapProxyBase<LinearSearchMap>::MapProxyBase;
};

template <>
class MapProxy<StdUnorderedMap> final : public MapProxyBase<StdUnorderedMap> {
 public:
  using MapProxyBase<StdUnorderedMap>::MapProxyBase;

  // NOLINTNEXTLINE
  auto Find(std::string_view key) { return Get().find(std::string{key}); }

  // NOLINTNEXTLINE
  auto FindPredefined(const http::headers::PredefinedHeader& header) {
    return Find(header);
  }

  // NOLINTNEXTLINE
  auto Erase(std::string_view key) { Get().erase(std::string{key}); }
};

constexpr http::headers::PredefinedHeader kArbitraryHeaders[] = {
    http::headers::kXYaTraceId,
    http::headers::kXYaSpanId,
    http::headers::kXYaRequestId,
    http::headers::kXRequestId,
    http::headers::kXBackendServer,
    http::headers::kXTaxiEnvoyProxyDstVhost,
    http::headers::kContentLength,
    http::headers::kDate,
    http::headers::kContentType,
    http::headers::kConnection,
    http::headers::kXYaTaxiClientTimeoutMs,
    http::headers::PredefinedHeader{"266e0c122bfa4ab38db2e65a789548cc"},
    http::headers::PredefinedHeader{"2b60ded9e0fd473f9a0050399a06d5f3"},
    http::headers::PredefinedHeader{"2218e6f0d5d7418aa950378a58a1c09e"},
    http::headers::PredefinedHeader{"82fa23011cab468f8cc2113c0d7f1262"},
    http::headers::PredefinedHeader{"10c9b8dd939b4fa6820faff32153fbe6"},
    http::headers::PredefinedHeader{"17a5a7dc35074daab1d92ace7185be63"},
    http::headers::PredefinedHeader{"5c3d52516ec849138947b9cc09eeddb0"},
    http::headers::PredefinedHeader{"cff017f8a3e24932a21f35e96f5232e1"},
    http::headers::PredefinedHeader{"47aa73b31ad244809a440576c488c73a"},
    http::headers::PredefinedHeader{"5c5ba42cc0b146d88005c8bdaef80960"},
    http::headers::PredefinedHeader{"294c05240a8b4b84ae826a335a1a44f8"},
    http::headers::PredefinedHeader{"40960a53827b4809a89e07c3d880fe68"},
    http::headers::PredefinedHeader{"b5f1dadbf5034a72ac5f032bd5120144"},
    http::headers::PredefinedHeader{"dd15a63215a44d0b86164a387f32b642"},
    http::headers::PredefinedHeader{"0de3ba12c07f4895856be747f3fcffa7"},
    http::headers::PredefinedHeader{"3eed1f273e0d4bbe961eae500d2bf34a"},
    http::headers::PredefinedHeader{"f1bbfac40332491aba57546456c630df"},
    http::headers::PredefinedHeader{"d7cd1146f69b417fae65bb809c1bb781"},
    http::headers::PredefinedHeader{"eab7f19c876f48828d443fb7abc4b208"},
    http::headers::PredefinedHeader{"f2b7420d06b94a9eb8622282a4c855a1"},
    http::headers::PredefinedHeader{"97e5bef911aa4bbb9898930d75e1fa57"},
    http::headers::PredefinedHeader{"bd3e282989d94ac4b4fab21d586e0a06"},
    http::headers::PredefinedHeader{"7f0b9c90bd304485b5c9f7d5ac4278ad"},
    http::headers::PredefinedHeader{"80e467dea2204956b09a90eec94c91e1"},
    http::headers::PredefinedHeader{"a496199a5c244d78815e3f20f5907a03"},
    http::headers::PredefinedHeader{"40bde39da5a842dfbb13b1a52cbafc98"},
    http::headers::PredefinedHeader{"b402e92d73c74bfcbfcf37b7a8f822d1"},
    http::headers::PredefinedHeader{"b4d453e134b945038ee6cac9a28b8243"},
    http::headers::PredefinedHeader{"14451de4db27482d97188b66cffb6ed9"},
    http::headers::PredefinedHeader{"0e0837f863054190842de4db37e47f8e"}};

constexpr http::headers::PredefinedHeader kAllUsedHeaders[] = {
    http::headers::kContentType,
    http::headers::kContentEncoding,
    http::headers::kContentLength,
    http::headers::kTransferEncoding,
    http::headers::kHost,
    http::headers::kAccept,
    http::headers::kAcceptEncoding,
    http::headers::kAcceptLanguage,
    http::headers::kApiKey,
    http::headers::kUserAgent,
    http::headers::kXRequestApplication,
    http::headers::kDate,
    http::headers::kWarning,
    http::headers::kAccessControlAllowHeaders,
    http::headers::kAllow,
    http::headers::kServer,
    http::headers::kSetCookie,
    http::headers::kConnection,
    http::headers::kCookie,
    http::headers::kXYaRequestId,
    http::headers::kXYaTraceId,
    http::headers::kXYaSpanId,
    http::headers::kXRequestId,
    http::headers::kXBackendServer,
    http::headers::kXTaxiEnvoyProxyDstVhost,
    http::headers::kXBaggage,
    http::headers::kXYaTaxiAllowAuthRequest,
    http::headers::kXYaTaxiAllowAuthResponse,
    http::headers::kXYaTaxiServerHostname,
    http::headers::kXYaTaxiClientTimeoutMs,
    http::headers::kXYaTaxiRatelimitedBy,
    http::headers::kXYaTaxiRatelimitReason,
};

template <typename Map>
void PopulateMap(MapProxy<Map>& proxy) {
  for (const auto& header : kArbitraryHeaders) {
    proxy.InsertOrAppend(std::string{header},
                         "we don't care, lets make it not small");
  }
}

template <typename Map>
void PopulateHugeMap(
    MapProxy<Map>& proxy,
    const std::vector<std::pair<std::string, std::string>>& headers) {
  for (const auto& [k, v] : headers) {
    proxy.InsertOrAppend(k, v);
  }
}

template <typename Map>
void PopulateMapWithKnown(MapProxy<Map>& proxy) {
  for (const auto& header : kAllUsedHeaders) {
    proxy.InsertOrAppend(std::string{header},
                         "we don't care, lets make it not small");
  }
}

}  // namespace

template <typename Map>
void HttpHeadersMap_Find(benchmark::State& state) {
  Map map{};
  MapProxy<Map> proxy{map};

  PopulateMap(proxy);
  for (auto _ : state) {
    for (const auto& header : kArbitraryHeaders) {
      benchmark::DoNotOptimize(proxy.Find(header));
    }
  }
}

template <typename Map>
void HttpHeadersMap_FindPredefined(benchmark::State& state) {
  Map map{};
  MapProxy<Map> proxy{map};

  PopulateMap(proxy);
  for (auto _ : state) {
    for (const auto& header : kArbitraryHeaders) {
      benchmark::DoNotOptimize(proxy.FindPredefined(header));
    }
  }
}

template <typename Map>
void HttpHeadersMap_Populate(benchmark::State& state) {
  for (auto _ : state) {
    Map map{};
    MapProxy<Map> proxy{map};

    proxy.Reserve(16);
    PopulateMap(proxy);

    benchmark::DoNotOptimize(map);
  }
}

template <typename Map>
void HttpHeadersMap_PopulateWithKnown(benchmark::State& state) {
  for (auto _ : state) {
    Map map{};
    MapProxy<Map> proxy{map};

    proxy.Reserve(16);
    PopulateMapWithKnown(proxy);

    benchmark::DoNotOptimize(map);
  }
}

template <typename Map, bool Predefined = false>
void HttpHeadersMap_FindHuge(benchmark::State& state) {
  constexpr std::size_t size = 10'000;

  std::vector<std::pair<std::string, std::string>> headers;
  headers.reserve(size);
  for (std::size_t i = 0; i < size; ++i) {
    headers.emplace_back(
        std::piecewise_construct,
        std::forward_as_tuple(std::string{"break_sso_break_sso_"} +
                              std::to_string(i)),
        std::forward_as_tuple(std::to_string(i) + "break_sso_break_sso_"));
  }
  std::vector<USERVER_NAMESPACE::http::headers::PredefinedHeader>
      Predefined_headers;
  if constexpr (Predefined) {
    Predefined_headers.reserve(headers.size());
    for (const auto& [k, v] : headers) {
      Predefined_headers.emplace_back(k);
    }
  }

  Map map{};
  MapProxy<Map> proxy{map};

  proxy.Reserve(headers.size());
  for (const auto& kvp : headers) {
    proxy.InsertOrAppend(kvp.first, kvp.second);
  }

  if constexpr (Predefined) {
    for (auto _ : state) {
      for (const auto& k : Predefined_headers) {
        benchmark::DoNotOptimize(proxy.FindPredefined(k));
      }
    }
  } else {
    for (auto _ : state) {
      for (const auto& [k, v] : headers) {
        const auto it = proxy.Find(k);
        if (it->second != v) {
          state.SkipWithError("Find is broken");
        }
      }
    }
  }
}

template <typename Map>
void HttpHeadersMap_PopulateHuge(benchmark::State& state) {
  constexpr std::size_t size = 10'000;

  std::vector<std::pair<std::string, std::string>> headers;
  headers.reserve(size);
  for (std::size_t i = 0; i < size; ++i) {
    headers.emplace_back(
        std::piecewise_construct,
        std::forward_as_tuple(std::string{"break_sso_break_sso_"} +
                              std::to_string(i)),
        std::forward_as_tuple(std::to_string(i) + "break_sso_break_sso_"));
  }

  for (auto _ : state) {
    Map map{};
    MapProxy<Map> proxy{map};

    proxy.Reserve(16);
    PopulateHugeMap<Map>(proxy, headers);
  }
}

template <typename Map>
void HttpHeadersMap_CopyAndEraseAll(benchmark::State& state) {
  Map map{};
  {
    MapProxy<Map> proxy{map};
    PopulateMapWithKnown(proxy);
  }

  for (auto _ : state) {
    auto map_copy = map;
    MapProxy<Map> proxy{map_copy};
    for (const auto& h : kAllUsedHeaders) {
      proxy.Erase(h);
    }
  }
}

namespace {

http::headers::PredefinedHeader MyLaunder(
    http::headers::PredefinedHeader value) {
  return Launder(value);
}

constexpr utils::TrivialBiMap kTrivialHeadersMap = [](auto selector) {
  return selector()
      .Case("content-type", 1)
      .Case("content-encoding", 2)
      .Case("content-length", 3)
      .Case("transfer-encoding", 4)
      .Case("host", 5)
      .Case("accept", 6)
      .Case("accept-encoding", 7)
      .Case("accept-language", 8)
      .Case("x-yataxi-api-key", 9)
      .Case("user-agent", 10)
      .Case("x-request-application", 11)
      .Case("date", 12)
      .Case("warning", 13)
      .Case("access-control-allow-headers", 14)
      .Case("allow", 15)
      .Case("server", 16)
      .Case("set-cookie", 17)
      .Case("connection", 18)
      .Case("cookie", 19)
      .Case("x-yarequestid", 20)
      .Case("x-yatraceid", 21)
      .Case("x-yaspanid", 22)
      .Case("x-requestid", 23)
      .Case("x-backend-server", 24)
      .Case("x-taxi-envoyproxy-dstvhost", 25)
      .Case("baggage", 26)
      .Case("x-yataxi-allow-auth-request", 27)
      .Case("x-yataxi-allow-auth-response", 28)
      .Case("x-yataxi-server-hostname", 29)
      .Case("x-yataxi-client-timeoutms", 30)
      .Case("x-yataxi-ratelimited-by", 31)
      .Case("x-yataxi-ratelimit-reason", 32);
};

void HttpHeadersMap_FindFromAllUsed_TrivialBiMap(benchmark::State& state) {
  std::vector<std::string_view> keys{};
  keys.reserve(std::size(kAllUsedHeaders));
  for (const auto& h : kAllUsedHeaders) {
    keys.push_back(MyLaunder(h));
  }

  for (auto _ : state) {
    for (const auto& h : keys) {
      benchmark::DoNotOptimize(kTrivialHeadersMap.TryFindICase(h));
    }
  }
}

void HttpHeadersMap_FindFromAllUsed_HeaderMap(benchmark::State& state) {
  HeaderMap map{};
  for (const auto& h : kAllUsedHeaders) {
    map.insert_or_assign(h, "some pretty long string to break soo");
  }

  std::vector<http::headers::PredefinedHeader> keys{};
  keys.reserve(std::size(kAllUsedHeaders));
  for (const auto& h : kAllUsedHeaders) {
    keys.push_back(MyLaunder(h));
  }

  for (auto _ : state) {
    for (const auto& h : keys) {
      benchmark::DoNotOptimize(map.find(h));
    }
  }
}

BENCHMARK(HttpHeadersMap_FindFromAllUsed_TrivialBiMap);
BENCHMARK(HttpHeadersMap_FindFromAllUsed_HeaderMap);

template <typename Map>
void HttpHeadersMap_Showcase(benchmark::State& state) {
  Map map{};
  MapProxy<Map> proxy{map};

  PopulateMapWithKnown(proxy);
  for (auto _ : state) {
    for (const auto& header : kAllUsedHeaders) {
      benchmark::DoNotOptimize(proxy.FindPredefined(header));
    }
  }
}

BENCHMARK_TEMPLATE(HttpHeadersMap_Showcase, LinearSearchMap);
BENCHMARK_TEMPLATE(HttpHeadersMap_Showcase, AbslMap);
BENCHMARK_TEMPLATE(HttpHeadersMap_Showcase, BoostFlatHashMap);
BENCHMARK_TEMPLATE(HttpHeadersMap_Showcase, StdUnorderedMap);
BENCHMARK_TEMPLATE(HttpHeadersMap_Showcase, HeaderMap);

}  // namespace

BENCHMARK_TEMPLATE(HttpHeadersMap_Find, LinearSearchMap);
BENCHMARK_TEMPLATE(HttpHeadersMap_Find, AbslMap);
BENCHMARK_TEMPLATE(HttpHeadersMap_Find, BoostFlatHashMap);
BENCHMARK_TEMPLATE(HttpHeadersMap_Find, StdUnorderedMap);
BENCHMARK_TEMPLATE(HttpHeadersMap_Find, HeaderMap);

BENCHMARK_TEMPLATE(HttpHeadersMap_FindPredefined, LinearSearchMap);
BENCHMARK_TEMPLATE(HttpHeadersMap_FindPredefined, AbslMap);
BENCHMARK_TEMPLATE(HttpHeadersMap_FindPredefined, BoostFlatHashMap);
BENCHMARK_TEMPLATE(HttpHeadersMap_FindPredefined, StdUnorderedMap);
BENCHMARK_TEMPLATE(HttpHeadersMap_FindPredefined, HeaderMap);

BENCHMARK_TEMPLATE(HttpHeadersMap_Populate, LinearSearchMap);
BENCHMARK_TEMPLATE(HttpHeadersMap_Populate, AbslMap);
BENCHMARK_TEMPLATE(HttpHeadersMap_Populate, BoostFlatHashMap);
BENCHMARK_TEMPLATE(HttpHeadersMap_Populate, StdUnorderedMap);
BENCHMARK_TEMPLATE(HttpHeadersMap_Populate, HeaderMap);

BENCHMARK_TEMPLATE(HttpHeadersMap_PopulateWithKnown, LinearSearchMap);
BENCHMARK_TEMPLATE(HttpHeadersMap_PopulateWithKnown, AbslMap);
BENCHMARK_TEMPLATE(HttpHeadersMap_PopulateWithKnown, BoostFlatHashMap);
BENCHMARK_TEMPLATE(HttpHeadersMap_PopulateWithKnown, StdUnorderedMap);
BENCHMARK_TEMPLATE(HttpHeadersMap_PopulateWithKnown, HeaderMap);

BENCHMARK_TEMPLATE(HttpHeadersMap_CopyAndEraseAll, LinearSearchMap);
BENCHMARK_TEMPLATE(HttpHeadersMap_CopyAndEraseAll, AbslMap);
BENCHMARK_TEMPLATE(HttpHeadersMap_CopyAndEraseAll, BoostFlatHashMap);
BENCHMARK_TEMPLATE(HttpHeadersMap_CopyAndEraseAll, StdUnorderedMap);
BENCHMARK_TEMPLATE(HttpHeadersMap_CopyAndEraseAll, HeaderMap);

BENCHMARK_TEMPLATE(HttpHeadersMap_FindHuge, AbslMap);
BENCHMARK_TEMPLATE(HttpHeadersMap_FindHuge, BoostFlatHashMap);
BENCHMARK_TEMPLATE(HttpHeadersMap_FindHuge, StdUnorderedMap);
BENCHMARK_TEMPLATE(HttpHeadersMap_FindHuge, HeaderMap);

constexpr bool kPredefined = true;
BENCHMARK_TEMPLATE(HttpHeadersMap_FindHuge, AbslMap, kPredefined);
BENCHMARK_TEMPLATE(HttpHeadersMap_FindHuge, BoostFlatHashMap, kPredefined);
BENCHMARK_TEMPLATE(HttpHeadersMap_FindHuge, StdUnorderedMap, kPredefined);
BENCHMARK_TEMPLATE(HttpHeadersMap_FindHuge, HeaderMap, kPredefined);

BENCHMARK_TEMPLATE(HttpHeadersMap_PopulateHuge, AbslMap);
BENCHMARK_TEMPLATE(HttpHeadersMap_PopulateHuge, BoostFlatHashMap);
BENCHMARK_TEMPLATE(HttpHeadersMap_PopulateHuge, StdUnorderedMap);
BENCHMARK_TEMPLATE(HttpHeadersMap_PopulateHuge, HeaderMap);

USERVER_NAMESPACE_END
#endif
