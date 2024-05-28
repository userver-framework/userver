#include <benchmark/benchmark.h>

#include <fmt/compile.h>
#include <sstream>

#include <userver/http/common_headers.hpp>
#include <userver/server/http/http_response.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/utils/small_string.hpp>

#include <server/http/http_request_impl.hpp>
#include <userver/server/request/response_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr std::string_view kCrlf = "\r\n";
constexpr std::string_view kKeyValueHeaderSeparator = ": ";

const server::http::HttpResponse::HeadersMap kHeaders = [] {
  server::http::HttpResponse::HeadersMap map{};
  for (std::size_t i = 1; i <= 7; ++i) {
    map[fmt::format("X-Header{}", i)] = "value";
  }
  return map;
}();

void OutputHeader(std::string& header, std::string_view key,
                  std::string_view val) {
  const auto old_size = header.size();
  header.resize(old_size + key.size() + kKeyValueHeaderSeparator.size() +
                val.size() + kCrlf.size());

  char* append_position = header.data() + old_size;
  const auto append = [&append_position](std::string_view what) {
    std::memcpy(append_position, what.data(), what.size());
    append_position += what.size();
  };

  append(key);
  append(kKeyValueHeaderSeparator);
  append(val);
  append(kCrlf);
}

void http_headers_serialization_inplace(benchmark::State& state) {
  for ([[maybe_unused]] auto _ : state) {
    USERVER_NAMESPACE::http::headers::HeadersString os;

    os.resize_and_overwrite(
        USERVER_NAMESPACE::http::headers::kTypicalHeadersSize,
        [&](char* data, std::size_t) {
          char* old_data_pointer = data;
          auto append = [&data](const std::string_view what) {
            std::memcpy(data, what.begin(), what.size());
            data += what.size();
          };
          append("HTTP/");
          data = fmt::format_to(data, FMT_COMPILE("{}.{} {} "), 1, 1, 200);
          append(StatusCodeString(server::http::HttpStatus::kOk));
          append("\r\n");
          return data - old_data_pointer;
        });

    kHeaders.OutputInHttpFormat(os);

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
    os.append(StatusCodeString(server::http::HttpStatus::kOk));
    os.append("\r\n");

    for (const auto& header : kHeaders) {
      OutputHeader(os, header.first, header.second);
    }

    if (kHeaders.find(USERVER_NAMESPACE::http::headers::kContentLength) ==
        kHeaders.end()) {
      OutputHeader(os, USERVER_NAMESPACE::http::headers::kContentLength,
                   fmt::format(FMT_COMPILE("{}"), 1024));
    }

    benchmark::DoNotOptimize(os);
  }
}

void http_headers_serialization_ostreams(benchmark::State& state) {
  for ([[maybe_unused]] auto _ : state) {
    std::ostringstream os;

    os << "HTTP/" << 1 << "." << 1 << " " << 200
       << StatusCodeString(server::http::HttpStatus::kOk)
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

void HttpResponseSetHeaderBenchmark(benchmark::State& state) {
  server::request::ResponseDataAccounter accounter{};
  const server::http::HttpRequestImpl request_impl{accounter};
  server::http::HttpResponse response{request_impl, accounter};

  namespace Headers = USERVER_NAMESPACE::http::headers;

  for ([[maybe_unused]] auto _ : state) {
    response.SetHeader(Headers::kDate, "Sat, 10 Feb 2024 12:12:35 UTC");
    response.SetHeader(Headers::kContentType, "application/octet-stream");
    response.SetHeader(Headers::kServer,
                       "userver/1.0.0 (20201109134848; rv:e20945c83fd)");
    response.SetHeader(Headers::kXYaRequestId,
                       "a3dd1efa7bf04e62902ba3283d24124f");
    response.SetHeader(Headers::kXYaTraceId,
                       "66971057871746f6808c8c62a68b28c4");
    response.SetHeader(Headers::kXYaSpanId, "6e77c7f1feb6ee42");
    response.SetHeader(Headers::kAcceptEncoding, "gzip, identity");
    response.SetHeader(Headers::kConnection, "keep-alive");
    response.SetHeader(Headers::kContentLength, "13");
    if (!response.ClearHeaders()) {
      std::abort();
    }
  }
}

}  // namespace

BENCHMARK(http_headers_serialization_inplace);
BENCHMARK(http_headers_serialization_no_ostreams);
BENCHMARK(http_headers_serialization_ostreams);
BENCHMARK(HttpResponseSetHeaderBenchmark);

USERVER_NAMESPACE_END
