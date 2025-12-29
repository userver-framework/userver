#include <server/http/http_request_parser.hpp>

#include <benchmark/benchmark.h>

#include <userver/engine/run_standalone.hpp>
#include <userver/http/http_version.hpp>

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

inline server::http::HttpRequestParser CreateBenchmarkParser(server::http::HttpRequestParser::OnNewRequestCb&& cb) {
    static const server::http::HandlerInfoIndex kTestHandlerInfoIndex;
    static const server::request::HttpRequestConfig kTestRequestConfig{
        /*.max_url_size = */ 8192,
        /*.max_request_size = */ 1024 * 1024,
        /*.max_headers_size = */ 65536,
        /*.request_body_size_log_limit = */ 512,
        /*.request_headers_size_log_limit = */ 512,
        /*.response_data_size_log_limit = */ 512,
        /*.parse_args_from_body = */ false,
        /*.testing_mode = */ true,  // non default value
        /*.decompress_request = */ false,
        /* set_tracing_headers = */ true,
        /* deadline_propagation_enabled = */ true,
        /* deadline_expired_status_code = */ server::http::HttpStatus{498}
    };
    static server::net::ParserStats test_stats;
    static server::request::ResponseDataAccounter test_accounter;
    return server::http::HttpRequestParser(
        kTestHandlerInfoIndex,
        kTestRequestConfig,
        std::move(cb),
        test_stats,
        test_accounter,
        engine::io::Sockaddr{}
    );
}

}  // namespace

void HttpRequestParserParseBenchmarkSmall(benchmark::State& state) {
    engine::RunStandalone([&] {
        auto parser = CreateBenchmarkParser([](std::shared_ptr<server::http::HttpRequest>&&) {});

        for ([[maybe_unused]] auto _ : state) {
            parser.Parse(kHttpRequestDataSmall);
        }
    });
}

void HttpRequestParserParseBenchmarkMiddle(benchmark::State& state) {
    engine::RunStandalone([&] {
        auto parser = CreateBenchmarkParser([](std::shared_ptr<server::http::HttpRequest>&&) {});

        for ([[maybe_unused]] auto _ : state) {
            parser.Parse(kHttpRequestDataMiddle);
        }
    });
}

void HttpRequestParserParseBenchmarkLargeUrl(benchmark::State& state) {
    engine::RunStandalone([&] {
        auto parser = CreateBenchmarkParser([](std::shared_ptr<server::http::HttpRequest>&&) {});

        std::string large_url;
        for (size_t i = 0; i < kEntryCount; ++i) {
            large_url += "/foo";
        }
        const std::string http_request_data = fmt::format("GET {} HTTP/1.1\r\n\r\n", large_url);

        for ([[maybe_unused]] auto _ : state) {
            parser.Parse(http_request_data);
        }
    });
}

void HttpRequestParserParseBenchmarkLargeBody(benchmark::State& state) {
    engine::RunStandalone([&] {
        auto parser = CreateBenchmarkParser([](std::shared_ptr<server::http::HttpRequest>&&) {});

        std::string large_body;
        for (size_t i = 0; i < kEntryCount; ++i) {
            large_body += "body";
        }
        const std::string http_request_data = fmt::format(
            "POST / HTTP/1.1\r\n"
            "Content-Length: {}\r\n\r\n{}",
            large_body.size(),
            large_body
        );

        for ([[maybe_unused]] auto _ : state) {
            parser.Parse(http_request_data);
        }
    });
}

void HttpRequestParserParseBenchmarkManyHeaders(benchmark::State& state) {
    engine::RunStandalone([&] {
        auto parser = CreateBenchmarkParser([](std::shared_ptr<server::http::HttpRequest>&&) {});

        std::string headers;
        for (size_t i = 0; i < kEntryCount; ++i) {
            headers += fmt::format("header{}: value\r\n", i);
        }
        const std::string http_request_data = fmt::format(
            "POST / HTTP/1.1\r\n"
            "{}\r\n\r\n",
            headers
        );

        for ([[maybe_unused]] auto _ : state) {
            parser.Parse(http_request_data);
        }
    });
}

BENCHMARK(HttpRequestParserParseBenchmarkSmall);
BENCHMARK(HttpRequestParserParseBenchmarkMiddle);
BENCHMARK(HttpRequestParserParseBenchmarkLargeUrl);
BENCHMARK(HttpRequestParserParseBenchmarkLargeBody);
BENCHMARK(HttpRequestParserParseBenchmarkManyHeaders);

USERVER_NAMESPACE_END
