#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>

#include <fmt/format.h>

#include <components/component_list_test.hpp>
#include <engine/io/tests/net_listener.hpp>
#include <server/net/connection_config.hpp>

#include <userver/clients/http/client.hpp>
#include <userver/clients/http/streamed_response.hpp>
#include <userver/components/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/components/run.hpp>
#include <userver/concurrent/queue.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/fs/blocking/file_descriptor.hpp>
#include <userver/server/component.hpp>
#include <userver/server/handlers/exceptions.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_response_body_stream.hpp>
#include <userver/utest/http_client.hpp>
#include <userver/utest/utest.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr auto kClientTimeout = utest::kMaxTestWaitTime;
constexpr auto kHeadersWaitTimeout = std::chrono::seconds{2};
constexpr auto kAbortCheckDelay = server::net::kDefaultAbortCheckDelay;

std::string GetHeader(const clients::http::Response& response, std::string_view name) {
    if (!response.headers().contains(name)) {
        return {};
    }
    return response.headers().at(name);
}

struct ListenerReservation final {
    std::uint16_t port{};
    fs::blocking::FileDescriptor fd;
};

ListenerReservation ReserveListener() {
    ListenerReservation result;
    engine::RunStandalone([&result] {
        const engine::io::tests::TcpListener listener;
        result.port = listener.Port();
        result.fd = fs::blocking::FileDescriptor::DupFd(listener.socket.Fd());
    });
    return result;
}

class HttpStreamHandler final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-http-stream";

    HttpStreamHandler(const components::ComponentConfig& config, const components::ComponentContext& context)
        : HttpHandlerBase(config, context)
    {}

    bool IsStreamed(const server::http::HttpRequest& request, server::request::RequestContext&) const override {
        ++is_streamed_calls_;
        const auto& mode = request.GetArg("mode");
        if (mode == "throw-in-is-streamed") {
            throw std::runtime_error("is-streamed");
        }
        if (mode == "throw-client-in-is-streamed") {
            throw server::handlers::ClientError(server::handlers::ExternalBody{"is-streamed-client"});
        }
        return mode != "not-streamed";
    }

    std::string HandleRequestThrow(const server::http::HttpRequest&, server::request::RequestContext&) const override {
        return "buffered-body";
    }

    void HandleStreamRequest(
        server::http::HttpRequest& request,
        server::request::RequestContext&,
        server::http::ResponseBodyStream& stream
    ) const override {
        const auto& mode = request.GetArg("mode");

        if (mode == "abort-check-delay") {
            engine::InterruptibleSleepFor(kAbortCheckDelay * 2);
            stream.SetStatusCode(server::http::HttpStatus::kOk);
            stream.SetHeader(std::string{"X-Stream-Test"}, std::string{"1"});
            stream.SetEndOfHeaders();
            stream.PushBodyChunk(std::string{"hello"}, engine::Deadline{});
            return;
        }

        if (mode == "throw-before-headers") {
            throw std::runtime_error("before-headers");
        }
        if (mode == "throw-client-before-headers") {
            throw server::handlers::ClientError(server::handlers::ExternalBody{"before-headers-client"});
        }

        if (mode == "slow-headers") {
            WaitForTester();
        }

        stream.SetStatusCode(server::http::HttpStatus::kOk);
        stream.SetHeader(std::string{"X-Stream-Test"}, std::string{"1"});

        if (mode == "no-set-end-of-headers-empty") {
            return;
        }

        stream.SetEndOfHeaders();

        if (mode == "throw-after-headers") {
            throw std::runtime_error("after-headers");
        }
        if (mode == "throw-client-after-headers") {
            throw server::handlers::ClientError(server::handlers::ExternalBody{"after-headers-client"});
        }
        if (mode == "headers-only") {
            return;
        }
        if (mode == "set-body") {
            stream.SetBody("full-body");
            return;
        }

        stream.PushBodyChunk(std::string{"hello"}, engine::Deadline{});

        if (mode == "hold-after-first-chunk") {
            holding_ = true;
            const bool resumed = WaitForTester();
            holding_ = false;
            if (resumed) {
                stream.PushBodyChunk(std::string{"-world"}, engine::Deadline{});
            }
            return;
        }

        if (mode == "throw-after-chunk") {
            WaitForTester();
            throw std::runtime_error("after-chunk");
        }
        if (mode == "throw-client-after-chunk") {
            WaitForTester();
            throw server::handlers::ClientError(server::handlers::ExternalBody{"after-chunk-client"});
        }

        stream.PushBodyChunk(std::string{"-world"}, engine::Deadline{});
    }

    engine::SingleConsumerEvent& ReadyEvent() const { return ready_event_; }
    engine::SingleConsumerEvent& ContinueEvent() const { return continue_event_; }
    bool IsHolding() const { return holding_; }
    int IsStreamedCalls() const { return is_streamed_calls_.load(); }

private:
    bool WaitForTester() const {
        ready_event_.Send();
        return continue_event_.WaitForEventFor(utest::kMaxTestWaitTime);
    }

    mutable engine::SingleConsumerEvent ready_event_;
    mutable engine::SingleConsumerEvent continue_event_;
    mutable std::atomic<bool> holding_{false};
    mutable std::atomic<int> is_streamed_calls_{0};
};

class HttpStreamTester final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "http-stream-tester";

    HttpStreamTester(const components::ComponentConfig& config, const components::ComponentContext& context)
        : ComponentBase(config, context),
          handler_(context.FindComponent<HttpStreamHandler>()),
          port_(config["port"].As<std::uint16_t>())
    {
        context.FindComponent<components::Server>();
    }

    void OnAllComponentsLoaded() override { RunTests(); }

    static yaml_config::Schema GetStaticConfigSchema() {
        return yaml_config::MergeSchemas<components::ComponentBase>(R"(
type: object
description: Runs HTTP streaming checks against the local server
additionalProperties: false
properties:
    port:
        type: integer
        description: listener port of the server under test
)");
    }

private:
    std::shared_ptr<clients::http::Response> Get(std::string_view mode) const {
        return client_->CreateRequest()
            .get(fmt::format("http://[::1]:{}/stream?mode={}", port_, mode))
            .timeout(kClientTimeout)
            .retry(1)
            .perform();
    }

    void ResumeHandler() const {
        ASSERT_TRUE(handler_.ReadyEvent().WaitForEventFor(kClientTimeout));
        handler_.ContinueEvent().Send();
    }

    clients::http::StreamedResponse StartStream(std::string_view mode) const {
        auto queue = concurrent::StringStreamQueue::Create();
        return client_->CreateRequest()
            .get(fmt::format("http://[::1]:{}/stream?mode={}", port_, mode))
            .timeout(kClientTimeout)
            .retry(1)
            .async_perform_stream_body(queue);
    }

    void RunTests() {
        client_ = utest::CreateHttpClient();

        TestOk();
        TestNotStreamed();
        TestSetBody();
        TestHeadersOnly();
        TestSlowHeaders();
        TestHeadersArriveBeforeHandlerFinishes();
        TestThrowInIsStreamed();
        TestClientThrowInIsStreamed();
        TestThrowBeforeHeaders();
        TestClientThrowBeforeHeaders();
        TestThrowAfterHeaders();
        TestClientThrowAfterHeaders();
        TestThrowAfterChunk();
        TestClientThrowAfterChunk();
        TestNoSetEndOfHeadersEmpty();
        TestIsStreamedCalledOncePerRequest();
    }

    void TestOk() {
        const auto response = Get("ok");
        EXPECT_EQ(response->status_code(), clients::http::Status::kOk);
        EXPECT_EQ(response->body(), "hello-world");
        EXPECT_EQ(GetHeader(*response, "X-Stream-Test"), "1");
    }

    void TestNotStreamed() {
        const auto response = Get("not-streamed");
        EXPECT_EQ(response->status_code(), clients::http::Status::kOk);
        EXPECT_EQ(response->body(), "buffered-body");
    }

    void TestSetBody() {
        const auto response = Get("set-body");
        EXPECT_EQ(response->status_code(), clients::http::Status::kOk);
        EXPECT_EQ(response->body(), "full-body");
    }

    void TestHeadersOnly() {
        const auto response = Get("headers-only");
        EXPECT_EQ(response->status_code(), clients::http::Status::kOk);
        EXPECT_TRUE(response->body().empty());
        EXPECT_EQ(GetHeader(*response, "X-Stream-Test"), "1");
    }

    void TestSlowHeaders() {
        auto unblock = engine::AsyncNoTracing([this] { ResumeHandler(); });
        const auto response = Get("slow-headers");
        unblock.Get();
        EXPECT_EQ(response->status_code(), clients::http::Status::kOk);
        EXPECT_EQ(response->body(), "hello-world");
    }

    void TestHeadersArriveBeforeHandlerFinishes() {
        auto streamed = StartStream("hold-after-first-chunk");

        EXPECT_EQ(streamed.StatusCode(), clients::http::Status::kOk);
        EXPECT_EQ(streamed.GetHeader("X-Stream-Test"), "1");

        std::string chunk;
        ASSERT_TRUE(streamed.ReadChunk(chunk, engine::Deadline::FromDuration(kHeadersWaitTimeout))
        ) << "First streamed chunk did not arrive while the handler is still running";
        EXPECT_EQ(chunk, "hello");
        EXPECT_TRUE(handler_.IsHolding()) << "Connection waited for the handler to finish instead of streaming";

        ResumeHandler();

        ASSERT_TRUE(streamed.ReadChunk(chunk, engine::Deadline::FromDuration(kHeadersWaitTimeout)));
        EXPECT_EQ(chunk, "-world");
        EXPECT_FALSE(handler_.IsHolding());
    }

    void TestThrowInIsStreamed() {
        const auto response = Get("throw-in-is-streamed");
        EXPECT_EQ(response->status_code(), clients::http::Status::kInternalServerError);
    }

    void TestClientThrowInIsStreamed() {
        const auto response = Get("throw-client-in-is-streamed");
        EXPECT_EQ(response->status_code(), clients::http::Status::kBadRequest);
        EXPECT_EQ(response->body(), "is-streamed-client");
    }

    void TestThrowBeforeHeaders() {
        const auto response = Get("throw-before-headers");
        EXPECT_EQ(response->status_code(), clients::http::Status::kInternalServerError);
    }

    void TestClientThrowBeforeHeaders() {
        const auto response = Get("throw-client-before-headers");
        EXPECT_EQ(response->status_code(), clients::http::Status::kBadRequest);
        EXPECT_EQ(response->body(), "before-headers-client");
    }

    void TestThrowAfterHeaders() {
        // SetEndOfHeaders() does not flush HTTP headers until the first chunk,
        // so exception middleware can still produce a regular error response.
        const auto response = Get("throw-after-headers");
        EXPECT_EQ(response->status_code(), clients::http::Status::kInternalServerError);
    }

    void TestClientThrowAfterHeaders() {
        const auto response = Get("throw-client-after-headers");
        EXPECT_EQ(response->status_code(), clients::http::Status::kBadRequest);
        EXPECT_EQ(response->body(), "after-headers-client");
    }

    void TestThrowAfterChunk() {
        auto streamed = StartStream("throw-after-chunk");

        EXPECT_EQ(streamed.StatusCode(), clients::http::Status::kOk);

        std::string chunk;
        ASSERT_TRUE(streamed.ReadChunk(chunk, engine::Deadline::FromDuration(kHeadersWaitTimeout)));
        EXPECT_EQ(chunk, "hello");

        ResumeHandler();

        EXPECT_FALSE(streamed.ReadChunk(chunk, engine::Deadline::FromDuration(kHeadersWaitTimeout)));
    }

    void TestClientThrowAfterChunk() {
        auto streamed = StartStream("throw-client-after-chunk");

        EXPECT_EQ(streamed.StatusCode(), clients::http::Status::kOk);

        std::string chunk;
        ASSERT_TRUE(streamed.ReadChunk(chunk, engine::Deadline::FromDuration(kHeadersWaitTimeout)));
        EXPECT_EQ(chunk, "hello");

        ResumeHandler();

        EXPECT_FALSE(streamed.ReadChunk(chunk, engine::Deadline::FromDuration(kHeadersWaitTimeout)));
    }

    void TestNoSetEndOfHeadersEmpty() {
        const auto response = Get("no-set-end-of-headers-empty");
        EXPECT_EQ(response->status_code(), clients::http::Status::kOk);
        EXPECT_TRUE(response->body().empty());
        EXPECT_EQ(GetHeader(*response, "X-Stream-Test"), "1");
    }

    void TestIsStreamedCalledOncePerRequest() {
        const auto calls_before = handler_.IsStreamedCalls();
        const auto response = Get("ok");
        EXPECT_EQ(response->status_code(), clients::http::Status::kOk);
        EXPECT_EQ(handler_.IsStreamedCalls(), calls_before + 1);
    }

    const HttpStreamHandler& handler_;
    const std::uint16_t port_;
    std::shared_ptr<clients::http::Client> client_;
};

}  // namespace

template <>
inline constexpr bool components::kHasValidate<HttpStreamTester> = true;

TEST_F(ComponentList, HttpStreaming) {
    const auto listener = ReserveListener();
    const auto config = tests::MergeYaml(
        tests::kMinimalStaticConfig,
        fmt::format(
            R"(
components_manager:
  task_processors:
    main-task-processor:
      worker_threads: 4
  components:
    server:
      listener:
        listen-socket-fd: {0}
        task_processor: main-task-processor
    handler-http-stream:
      path: /stream
      method: GET
      task_processor: main-task-processor
    http-stream-tester:
      port: {1}
)",
            listener.fd.GetNative(),
            listener.port
        )
    );

    components::RunOnce(
        components::InMemoryConfig{config},
        components::MinimalServerComponentList().Append<HttpStreamHandler>().Append<HttpStreamTester>()
    );
}

class AbortCheckDelayTester final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "abort-check-delay-tester";

    AbortCheckDelayTester(const components::ComponentConfig& config, const components::ComponentContext& context)
        : ComponentBase(config, context),
          port_(config["port"].As<std::uint16_t>())
    {
        context.FindComponent<components::Server>();
    }

    void OnAllComponentsLoaded() override {
        const auto client = utest::CreateHttpClient();

        auto queue = concurrent::StringStreamQueue::Create();
        auto streamed =
            client->CreateRequest()
                .get(fmt::format("http://[::1]:{}/stream?mode=abort-check-delay", port_))
                .timeout(kClientTimeout)
                .retry(1)
                .async_perform_stream_body(queue);

        const auto start = std::chrono::steady_clock::now();
        EXPECT_EQ(streamed.StatusCode(), clients::http::Status::kOk);
        const auto elapsed = std::chrono::steady_clock::now() - start;
        EXPECT_GE(elapsed, kAbortCheckDelay);
        EXPECT_EQ(streamed.GetHeader("X-Stream-Test"), "1");

        std::string chunk;
        ASSERT_TRUE(streamed.ReadChunk(chunk, engine::Deadline::FromDuration(kHeadersWaitTimeout)));
        EXPECT_EQ(chunk, "hello");
    }

    static yaml_config::Schema GetStaticConfigSchema() {
        return yaml_config::MergeSchemas<components::ComponentBase>(R"(
type: object
description: Checks that response headers wait for abort_check_delay before streaming starts
additionalProperties: false
properties:
    port:
        type: integer
        description: listener port of the server under test
)");
    }

private:
    const std::uint16_t port_;
};

template <>
inline constexpr bool components::kHasValidate<AbortCheckDelayTester> = true;

TEST_F(ComponentList, HttpStreamingHeadersAfterAbortCheckDelay) {
    const auto listener = ReserveListener();
    const auto config = tests::MergeYaml(
        tests::kMinimalStaticConfig,
        fmt::format(
            R"(
components_manager:
  task_processors:
    main-task-processor:
      worker_threads: 4
  components:
    server:
      listener:
        listen-socket-fd: {0}
        task_processor: main-task-processor
    handler-http-stream:
      path: /stream
      method: GET
      task_processor: main-task-processor
    abort-check-delay-tester:
      port: {1}
)",
            listener.fd.GetNative(),
            listener.port
        )
    );

    components::RunOnce(
        components::InMemoryConfig{config},
        components::MinimalServerComponentList().Append<HttpStreamHandler>().Append<AbortCheckDelayTester>()
    );
}

class PushChunkWithoutEndOfHeadersHandler final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-push-chunk-without-eoh";

    PushChunkWithoutEndOfHeadersHandler(
        const components::ComponentConfig& config,
        const components::ComponentContext& context
    )
        : HttpHandlerBase(config, context)
    {}

    bool IsStreamed(const server::http::HttpRequest&, server::request::RequestContext&) const override { return true; }

    void HandleStreamRequest(
        server::http::HttpRequest&,
        server::request::RequestContext&,
        server::http::ResponseBodyStream& stream
    ) const override {
        stream.SetStatusCode(server::http::HttpStatus::kOk);
        stream.PushBodyChunk(std::string{"hello"}, engine::Deadline{});
    }
};

class PushChunkWithoutEndOfHeadersTrigger final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "push-chunk-without-eoh-trigger";

    PushChunkWithoutEndOfHeadersTrigger(
        const components::ComponentConfig& config,
        const components::ComponentContext& context
    )
        : ComponentBase(config, context),
          port_(config["port"].As<std::uint16_t>())
    {
        context.FindComponent<components::Server>();
    }

    void OnAllComponentsLoaded() override {
        const auto client = utest::CreateHttpClient();
        const auto response =
            client->CreateRequest()
                .get(fmt::format("http://[::1]:{}/stream", port_))
                .timeout(kClientTimeout)
                .retry(1)
                .perform();
        EXPECT_EQ(response->status_code(), clients::http::Status::kOk);
        EXPECT_EQ(response->body(), "hello");
    }

    static yaml_config::Schema GetStaticConfigSchema() {
        return yaml_config::MergeSchemas<components::ComponentBase>(R"(
type: object
description: Triggers a streamed handler that skips SetEndOfHeaders()
additionalProperties: false
properties:
    port:
        type: integer
        description: listener port of the server under test
)");
    }

private:
    const std::uint16_t port_;
};

template <>
inline constexpr bool components::kHasValidate<PushChunkWithoutEndOfHeadersTrigger> = true;

using ComponentListDeathTest = ComponentList;

TEST_F(ComponentListDeathTest, HttpStreamingPushChunkWithoutSetEndOfHeaders) {
    testing::FLAGS_gtest_death_test_style = "threadsafe";

    const auto listener = ReserveListener();
    const auto config = tests::MergeYaml(
        tests::kMinimalStaticConfig,
        fmt::format(
            R"(
components_manager:
  task_processors:
    main-task-processor:
      worker_threads: 4
  components:
    server:
      listener:
        listen-socket-fd: {0}
        task_processor: main-task-processor
    handler-push-chunk-without-eoh:
      path: /stream
      method: GET
      task_processor: main-task-processor
    push-chunk-without-eoh-trigger:
      port: {1}
)",
            listener.fd.GetNative(),
            listener.port
        )
    );

    auto component_list =
        components::MinimalServerComponentList()
            .Append<PushChunkWithoutEndOfHeadersHandler>()
            .Append<PushChunkWithoutEndOfHeadersTrigger>();

    // TODO: looks like the UASSERT that the SetEndOfHeaders() lost its meaning as the server keeps working fine
#ifndef NDEBUG
    UEXPECT_DEATH(components::RunOnce(components::InMemoryConfig{config}, component_list), "SetEndOfHeaders");
#else
    components::RunOnce(components::InMemoryConfig{config}, component_list);
#endif
}

USERVER_NAMESPACE_END
