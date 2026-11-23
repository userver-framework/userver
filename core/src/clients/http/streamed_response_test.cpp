#include <userver/clients/http/streamed_response.hpp>

#include <array>
#include <chrono>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include <fmt/format.h>

#include <userver/clients/dns/exception.hpp>
#include <userver/clients/dns/resolver.hpp>
#include <userver/clients/http/client.hpp>
#include <userver/clients/http/client_core.hpp>
#include <userver/clients/http/error.hpp>
#include <userver/clients/http/request.hpp>
#include <userver/clients/http/response.hpp>
#include <userver/concurrent/queue.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/task/current_task.hpp>
#include <userver/engine/task/task_base.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/fs/temp_file.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/static_config/dns_client.hpp>
#include <userver/utest/dns_server_mock.hpp>
#include <userver/utest/http_client.hpp>
#include <userver/utest/simple_server.hpp>
#include <userver/utest/utest.hpp>

#include <curl-ev/error_code.hpp>
#include <engine/io/tests/net_listener.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

namespace {

constexpr std::string_view
    kPartialResponse = "HTTP/1.1 200 OK\r\nContent-Length: 14\r\nConnection: close\r\n\r\npartial";

engine::Deadline TestDeadline() { return engine::Deadline::FromDuration(utest::kMaxTestWaitTime); }

void ExpectBody(StreamedResponse& response, std::string_view expected) {
    const auto deadline = TestDeadline();
    std::string body;
    std::string chunk;
    // libcurl may split the body independently of the server's writes.
    while (body.size() < expected.size()) {
        ASSERT_TRUE(response.ReadChunk(chunk, deadline));
        ASSERT_FALSE(chunk.empty());
        body += chunk;
    }
    ASSERT_EQ(body, expected);
}

void ExpectPartialFile(StreamedResponse& response) {
    std::string chunk;
    try {
        response.ReadChunk(chunk, TestDeadline());
        FAIL() << "A truncated response must not end with a successful EOF";
    } catch (const TechnicalError& ex) {
        EXPECT_EQ(ex.error_code(), curl::errc::make_error_code(curl::errc::EasyErrorCode::kPartialFile));
    }
}

utest::SimpleServer::Response ReplyAfterHeaders(const std::string& request, std::string_view response) {
    if (request.find("\r\n\r\n") == std::string::npos) {
        return {{}, utest::SimpleServer::Response::kTryReadMore};
    }
    return {std::string{response}};
}

// SimpleServer cannot pause between writes without first reading another request.
class PausedResponseServer final {
public:
    explicit PausedResponseServer(std::string_view suffix = "-suffix")
        : task_(engine::AsyncNoTracing([this, suffix] {
              const auto deadline = TestDeadline();
              auto socket = listener_.socket.Accept(deadline);
              std::string request;
              std::array<char, 1024> buffer{};
              while (request.find("\r\n\r\n") == std::string::npos) {
                  const auto size = socket.RecvSome(buffer.data(), buffer.size(), deadline);
                  ASSERT_GT(size, 0);
                  request.append(buffer.data(), size);
              }
              ASSERT_EQ(
                  socket.SendAll(kPartialResponse.data(), kPartialResponse.size(), deadline),
                  kPartialResponse.size()
              );
              ASSERT_TRUE(finish_.WaitForEventUntil(deadline));
              if (!suffix.empty()) {
                  ASSERT_EQ(socket.SendAll(suffix.data(), suffix.size(), deadline), suffix.size());
              }
          })) {}

    std::string GetBaseUrl() const { return fmt::format("http://127.0.0.1:{}/", listener_.Port()); }

    void Finish() {
        finish_.Send();
        task_.WaitFor(utest::kMaxTestWaitTime);
        ASSERT_TRUE(task_.IsFinished());
        UASSERT_NO_THROW(task_.Get());
    }

private:
    engine::io::tests::TcpListener listener_{engine::io::tests::IpVersion::kV4};
    engine::SingleConsumerEvent finish_;
    engine::TaskWithResult<void> task_;
};

}  // namespace

UTEST(HttpStreamedResponse, ContentLengthUnderrunPreservesBody) {
    auto client = utest::CreateHttpClient();
    PausedResponseServer server{""};
    auto request = client->CreateRequest().get(server.GetBaseUrl()).timeout(utest::kMaxTestWaitTime);
    auto response = request.async_perform_stream_body(concurrent::StringStreamQueue::Create());
    ASSERT_EQ(response.StatusCode(), Status::kOk);
    ASSERT_NO_FATAL_FAILURE(ExpectBody(response, "partial"));

    // The connection is closed only after the caller has received part of the body.
    ASSERT_NO_FATAL_FAILURE(server.Finish());
    ExpectPartialFile(response);
    ExpectPartialFile(response);
}

UTEST(HttpStreamedResponse, TruncatedChunkedBody) {
    auto client = utest::CreateHttpClient();
    const utest::SimpleServer server{[](const auto& request) {
        return ReplyAfterHeaders(
            request,
            "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\nConnection: close\r\n\r\n7\r\npartial\r\n"
        );
    }};
    auto request = client->CreateRequest().get(server.GetBaseUrl()).timeout(utest::kMaxTestWaitTime);
    auto response = request.async_perform_stream_body(concurrent::StringStreamQueue::Create());
    ASSERT_EQ(response.StatusCode(), Status::kOk);
    ASSERT_NO_FATAL_FAILURE(ExpectBody(response, "partial"));
    ExpectPartialFile(response);
}

UTEST(HttpStreamedResponse, CompleteResponseReturnsEof) {
    struct TestCase {
        int status;
        std::string_view body;
    };
    constexpr std::array
        kCases{TestCase{200, "complete"}, TestCase{200, ""}, TestCase{404, "not found"}, TestCase{500, "server error"}};
    auto client = utest::CreateHttpClient();
    for (const auto& test : kCases) {
        SCOPED_TRACE(fmt::format("status={}, body='{}'", test.status, test.body));
        const utest::SimpleServer server{[&test](const auto& request) {
            return ReplyAfterHeaders(
                request,
                fmt::format(
                    "HTTP/1.1 {} Test\r\nContent-Length: {}\r\nConnection: close\r\n\r\n{}",
                    test.status,
                    test.body.size(),
                    test.body
                )
            );
        }};
        auto request = client->CreateRequest().get(server.GetBaseUrl()).timeout(utest::kMaxTestWaitTime);
        auto response = request.async_perform_stream_body(concurrent::StringStreamQueue::Create());
        ASSERT_EQ(static_cast<int>(response.StatusCode()), test.status);
        ASSERT_NO_FATAL_FAILURE(ExpectBody(response, test.body));
        std::string chunk;
        EXPECT_FALSE(response.ReadChunk(chunk, TestDeadline()));
    }
}

UTEST(HttpStreamedResponse, ReadDeadlineAllowsFurtherReading) {
    auto client = utest::CreateHttpClient();
    PausedResponseServer server;
    auto request = client->CreateRequest().get(server.GetBaseUrl()).timeout(utest::kMaxTestWaitTime);
    auto response = request.async_perform_stream_body(concurrent::StringStreamQueue::Create());
    ASSERT_NO_FATAL_FAILURE(ExpectBody(response, "partial"));

    auto reader = engine::AsyncNoTracing([&response] {
        std::string chunk;
        return response.ReadChunk(chunk, engine::Deadline::FromDuration(std::chrono::milliseconds{10}));
    });
    // The server is still paused, and the transfer timeout is longer than this safety timeout.
    reader.WaitFor(utest::kMaxTestWaitTime / 2);
    ASSERT_TRUE(reader.IsFinished());
    EXPECT_FALSE(reader.Get());

    ASSERT_NO_FATAL_FAILURE(server.Finish());
    ASSERT_NO_FATAL_FAILURE(ExpectBody(response, "-suffix"));
    std::string chunk;
    EXPECT_FALSE(response.ReadChunk(chunk, TestDeadline()));
}

UTEST(HttpStreamedResponse, CancelledReaderDoesNotCancelTransfer) {
    auto client = utest::CreateHttpClient();
    PausedResponseServer server;
    auto request = client->CreateRequest().get(server.GetBaseUrl()).timeout(utest::kMaxTestWaitTime);
    auto response = request.async_perform_stream_body(concurrent::StringStreamQueue::Create());
    ASSERT_NO_FATAL_FAILURE(ExpectBody(response, "partial"));

    engine::SingleConsumerEvent reading_started;
    auto reader = engine::AsyncNoTracing([&response, &reading_started] {
        std::string chunk;
        reading_started.Send();
        return response.ReadChunk(chunk, TestDeadline());
    });
    ASSERT_TRUE(reading_started.WaitForEventFor(utest::kMaxTestWaitTime));
    // UTEST has one worker: after Send(), the reader runs until ReadChunk suspends it.
    ASSERT_EQ(reader.GetState(), engine::TaskBase::State::kSuspended);
    reader.RequestCancel();
    reader.WaitFor(utest::kMaxTestWaitTime / 2);
    ASSERT_TRUE(reader.IsFinished());
    EXPECT_FALSE(reader.Get());

    ASSERT_NO_FATAL_FAILURE(server.Finish());
    ASSERT_NO_FATAL_FAILURE(ExpectBody(response, "-suffix"));
    std::string chunk;
    EXPECT_FALSE(response.ReadChunk(chunk, TestDeadline()));
}

UTEST(HttpStreamedResponse, TransferTimeoutAfterBody) {
    auto client = utest::CreateHttpClient();
    PausedResponseServer server{""};
    auto request = client->CreateRequest().get(server.GetBaseUrl()).timeout(std::chrono::seconds{1});
    auto response = request.async_perform_stream_body(concurrent::StringStreamQueue::Create());
    ASSERT_NO_FATAL_FAILURE(ExpectBody(response, "partial"));

    std::string chunk;
    // Unlike a ReadChunk deadline, the transfer timeout is an HTTP-client exception.
    UEXPECT_THROW(response.ReadChunk(chunk, TestDeadline()), TimeoutException);
    ASSERT_NO_FATAL_FAILURE(server.Finish());
}

UTEST(HttpStreamedResponse, ConfiguredResolverErrorClosesStream) {
    engine::SingleConsumerEvent dns_query_received;
    const utest::DnsServerMock dns_mock{[&dns_query_received](const auto&) -> utest::DnsServerMock::DnsAnswerVector {
        dns_query_received.Send();
        throw std::runtime_error("DNS SERVFAIL");
    }};
    auto& fs_task_processor = engine::current_task::GetBlockingTaskProcessor();
    const auto hosts_file = fs::TempFile::Create(fs_task_processor);
    ::userver::static_config::DnsClient config;
    config.hosts_file_path = hosts_file.GetPath();
    config.hosts_file_update_interval = utest::kMaxTestWaitTime;
    config.network_timeout = utest::kMaxTestWaitTime;
    config.network_attempts = 1;
    config.network_custom_servers = {dns_mock.GetServerAddress()};
    config.cache_max_reply_ttl = std::chrono::seconds{1};
    config.cache_failure_ttl = std::chrono::seconds{1};
    config.cache_ways = 1;
    config.cache_size_per_way = 1;
    clients::dns::Resolver resolver{fs_task_processor, config};
    auto client = utest::impl::CreateHttpClientCore(fs_task_processor);
    client->SetDnsResolver(&resolver);
    auto request = client->CreateRequest().get("http://streamed-response.test./").timeout(utest::kMaxTestWaitTime);
    auto queue = concurrent::StringStreamQueue::Create();
    std::optional<StreamedResponse> response;
    UASSERT_NO_THROW(response.emplace(request.async_perform_stream_body(queue)));

    UEXPECT_THROW(response->GetHeaders(), clients::dns::ResolverException);
    EXPECT_TRUE(dns_query_received.IsReady());
    EXPECT_TRUE(queue->NoMoreProducers());
}

UTEST(HttpStreamedResponse, ExpiredInheritedDeadlineClosesStream) {
    auto client = utest::CreateHttpClient();
    const utest::SimpleServer http_server{[](const auto&) {
        ADD_FAILURE() << "An already-expired request must not be sent";
        return utest::SimpleServer::Response{};
    }};
    auto request = client->CreateRequest().get(http_server.GetBaseUrl()).timeout(utest::kMaxTestWaitTime);
    auto queue = concurrent::StringStreamQueue::Create();
    server::request::TaskInheritedData data;
    data.deadline = engine::Deadline::Passed();
    server::request::kTaskInheritedData.Set(std::move(data));

    // Complete runs before StreamedResponse retrieves the body future.
    std::optional<StreamedResponse> response;
    UASSERT_NO_THROW(response.emplace(request.async_perform_stream_body(queue)));
    UEXPECT_THROW(response->GetHeaders(), CancelException);
    EXPECT_TRUE(queue->NoMoreProducers());
}

UTEST(HttpStreamedResponse, RequestReusePreservesUnreadBodyAndError) {
    auto client = utest::CreateHttpClient();
    unsigned requests = 0;
    const utest::SimpleServer server{[&requests](const auto& request) {
        if (request.find("\r\n\r\n") == std::string::npos) {
            return utest::SimpleServer::Response{{}, utest::SimpleServer::Response::kTryReadMore};
        }
        return ReplyAfterHeaders(
            request,
            ++requests == 1 ? kPartialResponse : "HTTP/1.1 200 OK\r\nContent-Length: 2\r\nConnection: close\r\n\r\nok"
        );
    }};
    auto request = client->CreateRequest().get(server.GetBaseUrl()).timeout(utest::kMaxTestWaitTime);
    auto old_response = request.async_perform_stream_body(concurrent::StringStreamQueue::Create());
    ASSERT_EQ(old_response.StatusCode(), Status::kOk);

    // Reusing Request waits for completion, but must not reset the old stream's queued data or error.
    auto new_response = request.async_perform_stream_body(concurrent::StringStreamQueue::Create());
    ASSERT_EQ(new_response.StatusCode(), Status::kOk);
    ASSERT_NO_FATAL_FAILURE(ExpectBody(new_response, "ok"));
    std::string chunk;
    EXPECT_FALSE(new_response.ReadChunk(chunk, TestDeadline()));

    ASSERT_NO_FATAL_FAILURE(ExpectBody(old_response, "partial"));
    ExpectPartialFile(old_response);
}

}  // namespace clients::http

USERVER_NAMESPACE_END
