#include <atomic>
#include <barrier>
#include <exception>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include <userver/http/common_headers.hpp>
#include <userver/logging/level.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/http/http_request_builder.hpp>
#include <userver/tracing/manager.hpp>
#include <userver/tracing/opentelemetry.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/tracer.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

namespace {

utils::Flags<Format> GetFormats() { return {Format::kYandexTaxi, Format::kOpenTelemetry}; }

}  // namespace

UTEST(GenericTracingManager, HiddenSpanDoesNotProduceResponseHeaders) {
    SetInheritedOtelTracingData({}, "00");
    auto span = Span::MakeRootSpan("test-span");
    span.SetLogLevel(logging::Level::kNone);
    const GenericTracingManager manager{GetFormats(), Format{}};
    auto request = server::http::HttpRequestBuilder{}.Build();

    manager.FillResponseWithTracingContext(span, request->GetHttpResponse());

    const auto& response = request->GetHttpResponse();
    EXPECT_TRUE(response.GetHeader(http::headers::kXYaTraceId).empty());
    EXPECT_TRUE(response.GetHeader(http::headers::opentelemetry::kTraceParent).empty());
}

UTEST(GenericTracingManager, IndependentSpanLogLevelKeepsResponseHeaders) {
    const TracingStateGuard tracing_state_guard;
    const logging::DefaultLoggerLevelScope default_logger_level{logging::Level::kWarning};
    SetSpanLogLevel(logging::Level::kInfo);
    SetInheritedOtelTracingData({}, "00");
    auto span = Span::MakeRootSpan("test-span");
    const GenericTracingManager manager{GetFormats(), Format{}};
    auto request = server::http::HttpRequestBuilder{}.Build();

    manager.FillResponseWithTracingContext(span, request->GetHttpResponse());

    const auto& response = request->GetHttpResponse();
    EXPECT_EQ(response.GetHeader(http::headers::kXYaTraceId), span.GetTraceId());
    EXPECT_EQ(response.GetHeader(http::headers::kXYaSpanId), span.GetSpanId());
    EXPECT_EQ(response.GetHeader(http::headers::kXYaRequestId), span.GetLink());
    const auto traceparent =
        opentelemetry::ExtractTraceParentDataView(response.GetHeader(http::headers::opentelemetry::kTraceParent));
    ASSERT_TRUE(traceparent.has_value());
    EXPECT_EQ(traceparent.value().span_id, span.GetSpanId());
}

TEST(TracingConfiguration, ConcurrentSpanLogLevelAccess) {
    constexpr std::size_t kThreadCount = 8;
    constexpr std::size_t kIterationCount = 10'000;
    constexpr logging::Level kFirstLevel = logging::Level::kInfo;
    constexpr logging::Level kSecondLevel = logging::Level::kWarning;

    const TracingStateGuard tracing_state_guard;
    SetSpanLogLevel(kFirstLevel);
    std::barrier start{kThreadCount};
    std::atomic<bool> has_unexpected_level{false};
    std::vector<std::thread> threads;
    threads.reserve(kThreadCount);

    for (std::size_t thread_index = 0; thread_index < kThreadCount; ++thread_index) {
        threads.emplace_back([&, thread_index] {
            start.arrive_and_wait();
            for (std::size_t iteration = 0; iteration < kIterationCount; ++iteration) {
                const auto level = (thread_index + iteration) % 2 == 0 ? kFirstLevel : kSecondLevel;
                SetSpanLogLevel(level);
                const auto current_level = GetSpanLogLevel();
                if (current_level != kFirstLevel && current_level != kSecondLevel) {
                    has_unexpected_level.store(true);
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_FALSE(has_unexpected_level.load());
}

UTEST(TracingConfiguration, StateGuardResetsAndRestoresState) {
    constexpr const char* kInitiallyIgnoredSpan = "initially-ignored-span";
    constexpr const char* kInitiallyIgnoredPrefix = "initially-ignored-prefix/";
    constexpr const char* kInitiallyIgnoredPrefixedSpan = "initially-ignored-prefix/child";
    constexpr const char* kScopedIgnoredSpan = "scoped-ignored-span";
    constexpr const char* kScopedIgnoredPrefix = "scoped-ignored-prefix/";
    constexpr const char* kScopedIgnoredPrefixedSpan = "scoped-ignored-prefix/child";

    const TracingStateGuard test_state_guard;
    NoLogSpans no_log_spans;
    no_log_spans.names.emplace(kInitiallyIgnoredSpan);
    no_log_spans.prefixes.emplace(kInitiallyIgnoredPrefix);
    SetNoLogSpans(std::move(no_log_spans));
    SetSpanLogLevel(logging::Level::kWarning);

    {
        const TracingStateGuard tracing_state_guard;
        EXPECT_FALSE(IsNoLogSpan(kInitiallyIgnoredSpan));
        EXPECT_FALSE(IsNoLogSpan(kInitiallyIgnoredPrefixedSpan));
        EXPECT_EQ(GetSpanLogLevel(), std::nullopt);

        NoLogSpans scoped_no_log_spans;
        scoped_no_log_spans.names.emplace(kScopedIgnoredSpan);
        scoped_no_log_spans.prefixes.emplace(kScopedIgnoredPrefix);
        SetNoLogSpans(std::move(scoped_no_log_spans));
        SetSpanLogLevel(logging::Level::kNone);

        {
            const TracingStateGuard nested_state_guard;
            EXPECT_FALSE(IsNoLogSpan(kScopedIgnoredSpan));
            EXPECT_FALSE(IsNoLogSpan(kScopedIgnoredPrefixedSpan));
            EXPECT_EQ(GetSpanLogLevel(), std::nullopt);
            SetSpanLogLevel(logging::Level::kInfo);
        }

        EXPECT_TRUE(IsNoLogSpan(kScopedIgnoredSpan));
        EXPECT_TRUE(IsNoLogSpan(kScopedIgnoredPrefixedSpan));
        EXPECT_FALSE(IsNoLogSpan(kInitiallyIgnoredSpan));
        EXPECT_FALSE(IsNoLogSpan(kInitiallyIgnoredPrefixedSpan));
        EXPECT_EQ(GetSpanLogLevel(), logging::Level::kNone);
    }

    EXPECT_TRUE(IsNoLogSpan(kInitiallyIgnoredSpan));
    EXPECT_TRUE(IsNoLogSpan(kInitiallyIgnoredPrefixedSpan));
    EXPECT_FALSE(IsNoLogSpan(kScopedIgnoredSpan));
    EXPECT_FALSE(IsNoLogSpan(kScopedIgnoredPrefixedSpan));
    EXPECT_EQ(GetSpanLogLevel(), logging::Level::kWarning);
}

UTEST(TracingConfiguration, StateGuardRestoresEmptyState) {
    constexpr const char* kScopedIgnoredSpan = "scoped-ignored-span";
    const TracingStateGuard test_state_guard;

    {
        const TracingStateGuard tracing_state_guard;
        NoLogSpans no_log_spans;
        no_log_spans.names.emplace(kScopedIgnoredSpan);
        SetNoLogSpans(std::move(no_log_spans));
        SetSpanLogLevel(logging::Level::kInfo);
    }

    EXPECT_FALSE(IsNoLogSpan(kScopedIgnoredSpan));
    EXPECT_EQ(GetSpanLogLevel(), std::nullopt);
}

UTEST(TracingConfiguration, StateGuardRestoresStateOnException) {
    constexpr const char* kInitiallyIgnoredSpan = "initially-ignored-span";
    struct TestException final : std::exception {};
    const TracingStateGuard test_state_guard;
    NoLogSpans no_log_spans;
    no_log_spans.names.emplace(kInitiallyIgnoredSpan);
    SetNoLogSpans(std::move(no_log_spans));
    SetSpanLogLevel(logging::Level::kWarning);

    EXPECT_THROW(
        {
            const TracingStateGuard tracing_state_guard;
            SetSpanLogLevel(logging::Level::kInfo);
            throw TestException{};
        },
        TestException
    );

    EXPECT_TRUE(IsNoLogSpan(kInitiallyIgnoredSpan));
    EXPECT_EQ(GetSpanLogLevel(), logging::Level::kWarning);
}

}  // namespace tracing

USERVER_NAMESPACE_END
