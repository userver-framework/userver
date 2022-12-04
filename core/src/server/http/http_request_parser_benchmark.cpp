#include <benchmark/benchmark.h>

#include <server/http/http_request_parser.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr std::string_view kHttpRequestData =
    "POST /hello HTTP/1.1\r\n"
    "Host: localhost:11235\r\nUser-Agent: curl/7.58.0\r\nAccept: */*\r\n"
    "Header1: Value1\r\nHeader2: Value2\r\nHeader3: Value3\r\n"
    "Content-type: application/json\r\nContent-Length: 18\r\n\r\n"
    "{\"hello\": \"world\"}";

constexpr std::string_view kHeaders[] = {
    "user-agent", "accept",       "header1",       "header2",
    "header3",    "content-type", "content-length"};

constexpr size_t kContentLength = 18;

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

void ValidateParser(benchmark::State& state) {
  bool parsed = false;
  auto parser = CreateBenchmarkParser(
      [&parsed,
       &state](std::shared_ptr<server::request::RequestBase>&& request) {
        parsed = true;
        auto& http_request_impl =
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
            static_cast<server::http::HttpRequestImpl&>(*request);

        if (http_request_impl.GetMethod() != server::http::HttpMethod::kPost) {
          state.SkipWithError("Failed to parse http method");
        }

        for (const auto header : kHeaders) {
          if (!http_request_impl.HasHeader(std::string{header})) {
            state.SkipWithError("Failed to parse http headers");
          }
        }

        if (http_request_impl.RequestBody().size() != kContentLength) {
          state.SkipWithError("Failed to parse http body");
        }
      });

  parser.Parse(kHttpRequestData.data(), kHttpRequestData.size());
  if (!parsed) {
    state.SkipWithError("Failed to parse http request");
  }
}

}  // namespace

void http_request_parser_parse_benchmark(benchmark::State& state) {
  ValidateParser(state);

  auto parser = CreateBenchmarkParser(
      [](std::shared_ptr<server::request::RequestBase>&&) {});

  for (auto _ : state) {
    parser.Parse(kHttpRequestData.data(), kHttpRequestData.size());
  }
}
BENCHMARK(http_request_parser_parse_benchmark);

USERVER_NAMESPACE_END
