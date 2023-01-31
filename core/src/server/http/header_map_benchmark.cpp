#include <benchmark/benchmark.h>

#include <userver/server/http/header_map.hpp>

#include <server/http/header_map_impl/special_header.hpp>

#include <userver/utils/str_icase.hpp>

USERVER_NAMESPACE_BEGIN

namespace {
using HeaderMap = server::http::HeaderMap;
using OldMap = std::unordered_map<std::string, std::string, utils::StrIcaseHash,
                                  utils::StrIcaseEqual>;

template <typename Map>
class MapWrapper final {
 public:
  MapWrapper(Map& map) : map_{map} {}

  void Insert(server::http::SpecialHeader key, std::string value) {
    map_.emplace(key.name, std::move(value));
  }

  void Erase(server::http::SpecialHeader key) {
    map_.erase(std::string{key.name});
  }

  auto find(server::http::SpecialHeader key) {
    return map_.find(std::string{key.name});
  }

  auto end() { return map_.end(); }

  static Map Create() { return Map(32); }

  Map& Get() { return map_; }

 private:
  Map& map_;
};

template <>
class MapWrapper<HeaderMap> final {
 public:
  MapWrapper(HeaderMap& map) : map_{map} {}

  void Insert(server::http::SpecialHeader key, std::string value) {
    map_.Insert(key, std::move(value));
  }

  void Erase(server::http::SpecialHeader key) { map_.Erase(key); }

  auto find(server::http::SpecialHeader key) { return map_.find(key); }

  auto end() { return map_.end(); }

  static HeaderMap Create() { return {}; }

  HeaderMap& Get() { return map_; }

 private:
  HeaderMap& map_;
};

template <typename Wrapper>
void PrepareResponse(Wrapper& headers) {
  headers.Insert(server::http::kXYaTraceIdHeader,
                 "18575ca478104b9fb81460875a2e0a2c");
  headers.Insert(server::http::kXYaSpanIdHeader, "dfea18ffa9c24597");
  headers.Insert(server::http::kXYaRequestIdHeader,
                 "767e8d274faf489789cda8a030a98361");
  headers.Insert(server::http::kServerHeader, "userver at some version");
}

}  // namespace

template <typename Map>
void header_map_for_response(benchmark::State& state) {
  for (auto _ : state) {
    Map headers = MapWrapper<Map>::Create();
    auto wrapper = MapWrapper<Map>{headers};

    Map request_header = MapWrapper<Map>::Create();
    auto request_wrapper = MapWrapper<Map>{request_header};

    request_wrapper.Insert(server::http::kContentLengthHeader, "1");

    benchmark::DoNotOptimize(
        request_wrapper.find(server::http::kXYaRequestIdHeader));
    benchmark::DoNotOptimize(
        request_wrapper.find(server::http::kXYaTraceIdHeader));
    benchmark::DoNotOptimize(
        request_wrapper.find(server::http::kXYaSpanIdHeader));

    benchmark::DoNotOptimize(
        request_wrapper.find(server::http::kXRequestIdHeader));
    benchmark::DoNotOptimize(
        request_wrapper.find(server::http::kXBackendServerHeader));
    benchmark::DoNotOptimize(
        request_wrapper.find(server::http::kXTaxiEnvoyProxyDstVhostHeader));

    PrepareResponse(wrapper);

    wrapper.Erase(server::http::kContentLengthHeader);
    const auto end = wrapper.end();

    benchmark::DoNotOptimize(wrapper.find(server::http::kDateHeader) == end);
    benchmark::DoNotOptimize(wrapper.find(server::http::kContentTypeHeader) ==
                             end);

    std::size_t total_size = 0;
    for (const auto& item : wrapper.Get()) {
      total_size += item.first.size() + item.second.size();
    }
    benchmark::DoNotOptimize(total_size);

    benchmark::DoNotOptimize(wrapper.find(server::http::kConnectionHeader) ==
                             end);

    benchmark::ClobberMemory();
  }
}
BENCHMARK_TEMPLATE(header_map_for_response, HeaderMap);
BENCHMARK_TEMPLATE(header_map_for_response, OldMap);

USERVER_NAMESPACE_END
