#include <server/http/http_request_parser.hpp>

#include <benchmark/benchmark.h>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr std::string_view kHttpRequestDataSmall = "GET / HTTP/1.1\r\n\r\n";

constexpr std::string_view kHttpRequestDataMiddle =
    "POST /hello HTTP/1.1\r\n"
    "Host: localhost:11235\r\nUser-Agent: curl/7.58.0\r\nAccept: */*\r\n"
    "Header1: Value1\r\nHeader2: Value2\r\nHeader3: Value3\r\n"
    "Content-type: application/json\r\nContent-Length: 18\r\n\r\n"
    "{\"hello\": \"world\"}";

constexpr size_t kEntryCount = 1024;

inline server::http::HttpRequestParser CreateBenchmarkParser(
    server::http::HttpRequestParser::OnNewRequestCb&& cb) {
  static const server::http::HandlerInfoIndex kTestHandlerInfoIndex;
  static constexpr server::request::HttpRequestConfig kTestRequestConfig{
      /*.max_url_size = */ 8192,
      /*.max_request_size = */ 1024 * 1024,
      /*.max_headers_size = */ 65536,
      /*.parse_args_from_body = */ false,
      /*.testing_mode = */ true,  // non default value
      /*.decompress_request = */ false,
  };
  static server::net::ParserStats test_stats;
  static server::request::ResponseDataAccounter test_accounter;
  return server::http::HttpRequestParser(kTestHandlerInfoIndex,
                                         kTestRequestConfig, std::move(cb),
                                         test_stats, test_accounter);
}

}  // namespace

void http_request_parser_parse_benchmark_small(benchmark::State& state) {
  auto parser = CreateBenchmarkParser(
      [](std::shared_ptr<server::request::RequestBase>&&) {});

  for ([[maybe_unused]] auto _ : state) {
    parser.Parse(kHttpRequestDataSmall.data(), kHttpRequestDataSmall.size());
  }
}

void http_request_parser_parse_benchmark_middle(benchmark::State& state) {
  auto parser = CreateBenchmarkParser(
      [](std::shared_ptr<server::request::RequestBase>&&) {});

  for ([[maybe_unused]] auto _ : state) {
    parser.Parse(kHttpRequestDataMiddle.data(), kHttpRequestDataMiddle.size());
  }
}

void http_request_parser_parse_benchmark_large_url(benchmark::State& state) {
  auto parser = CreateBenchmarkParser(
      [](std::shared_ptr<server::request::RequestBase>&&) {});

  std::string large_url;
  for (size_t i = 0; i < kEntryCount; ++i) {
    large_url += "/foo";
  }
  const std::string http_request_data =
      fmt::format("GET {} HTTP/1.1\r\n\r\n", large_url);

  for ([[maybe_unused]] auto _ : state) {
    parser.Parse(http_request_data.data(), http_request_data.size());
  }
}

void http_request_parser_parse_benchmark_large_body(benchmark::State& state) {
  auto parser = CreateBenchmarkParser(
      [](std::shared_ptr<server::request::RequestBase>&&) {});

  std::string large_body;
  for (size_t i = 0; i < kEntryCount; ++i) {
    large_body += "body";
  }
  const std::string http_request_data = fmt::format(
      "POST / HTTP/1.1\r\n"
      "Content-Length: {}\r\n\r\n{}",
      large_body.size(), large_body);

  for ([[maybe_unused]] auto _ : state) {
    parser.Parse(http_request_data.data(), http_request_data.size());
  }
}

void http_request_parser_parse_benchmark_many_headers(benchmark::State& state) {
  auto parser = CreateBenchmarkParser(
      [](std::shared_ptr<server::request::RequestBase>&&) {});

  std::string headers;
  for (size_t i = 0; i < kEntryCount; ++i) {
    headers += fmt::format("header{}: value\r\n", i);
  }
  const std::string http_request_data = fmt::format(
      "POST / HTTP/1.1\r\n"
      "{}\r\n\r\n",
      headers);

  for ([[maybe_unused]] auto _ : state) {
    parser.Parse(http_request_data.data(), http_request_data.size());
  }
}

BENCHMARK(http_request_parser_parse_benchmark_small);
BENCHMARK(http_request_parser_parse_benchmark_middle);
BENCHMARK(http_request_parser_parse_benchmark_large_url);
BENCHMARK(http_request_parser_parse_benchmark_large_body);
BENCHMARK(http_request_parser_parse_benchmark_many_headers);

USERVER_NAMESPACE_END
