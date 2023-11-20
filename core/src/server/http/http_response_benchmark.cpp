#include <benchmark/benchmark.h>

#include <fmt/compile.h>
#include <sstream>

#include <userver/http/common_headers.hpp>
#include <userver/server/http/http_response.hpp>
#include <userver/server/http/http_status.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

const server::http::HttpResponse::HeadersMap kHeaders = [] {
  server::http::HttpResponse::HeadersMap map{};
  for (std::size_t i = 1; i <= 7; ++i) {
    map[fmt::format("X-Header{}", i)] = "value";
  }
  return map;
}();

void http_headers_serialization_inplace(benchmark::State& state) {
  for ([[maybe_unused]] auto _ : state) {
    USERVER_NAMESPACE::http::headers::HeadersString os;

    const std::string_view kHttpString = "HTTP/";
    const std::string kProtocol =
        fmt::format(FMT_COMPILE("{}.{} {} "), 1, 1, 200);
    const std::string_view kStatusString =
        HttpStatusString(server::http::HttpStatus::kOk);
    const std::string_view kCrlf = "\r\n";

    os.resize_and_overwrite(
        kHttpString.size() + kProtocol.size() + kStatusString.size() +
            kCrlf.size(),
        [&](char* data, std::size_t size) {
          const auto append = [&data](std::string_view what) {
            std::memcpy(data, what.begin(), what.size());
            data += what.size();
          };
          append(kHttpString);
          append(kProtocol);
          append(kStatusString);
          append(kCrlf);
          return size;
        });

    for (const auto& header : kHeaders) {
      server::http::impl::OutputHeader(os, header.first, header.second);
    }

    if (kHeaders.find(USERVER_NAMESPACE::http::headers::kContentLength) ==
        kHeaders.end()) {
      server::http::impl::OutputHeader(
          os, USERVER_NAMESPACE::http::headers::kContentLength,
          fmt::format(FMT_COMPILE("{}"), 1024));
    }

    benchmark::DoNotOptimize(os);
  }
}

void http_headers_serialization_no_ostreams(benchmark::State& state) {
  for ([[maybe_unused]] auto _ : state) {
    std::string os;
    os.reserve(1024);

    os.append("HTTP/");
    fmt::format_to(std::back_inserter(os), FMT_COMPILE("{}.{} {} "), 1, 1, 200);
    os.append(HttpStatusString(server::http::HttpStatus::kOk));
    os.append("\r\n");

    for (const auto& header : kHeaders) {
      server::http::impl::OutputHeader(os, header.first, header.second);
    }

    if (kHeaders.find(USERVER_NAMESPACE::http::headers::kContentLength) ==
        kHeaders.end()) {
      server::http::impl::OutputHeader(
          os, USERVER_NAMESPACE::http::headers::kContentLength,
          fmt::format(FMT_COMPILE("{}"), 1024));
    }

    benchmark::DoNotOptimize(os);
  }
}

void http_headers_serialization_ostreams(benchmark::State& state) {
  for ([[maybe_unused]] auto _ : state) {
    std::ostringstream os;

    os << "HTTP/" << 1 << "." << 1 << " " << 200
       << server::http::HttpStatusString(server::http::HttpStatus::kOk)
       << "\r\n";

    for (const auto& header : kHeaders) {
      os << header.first << ": " << header.second << "\r\n";
    }

    if (kHeaders.find(USERVER_NAMESPACE::http::headers::kContentLength) ==
        kHeaders.end()) {
      os << std::string_view{USERVER_NAMESPACE::http::headers::kContentLength}
         << ": " << 1024 << "\r\n";
    }

    auto data = os.str();
    benchmark::DoNotOptimize(data);
  }
}

}  // namespace

BENCHMARK(http_headers_serialization_inplace);
BENCHMARK(http_headers_serialization_no_ostreams);
BENCHMARK(http_headers_serialization_ostreams);

USERVER_NAMESPACE_END
