#include <userver/clients/http/client.hpp>

#include <userver/clients/http/client_core.hpp>
#include <userver/clients/http/client_with_middlewares.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/logging/log.hpp>
#include <userver/utest/http_client.hpp>
#include <userver/utest/simple_server.hpp>
#include <userver/utest/stress.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using HttpResponse = utest::SimpleServer::Response;
using HttpRequest = utest::SimpleServer::Request;

constexpr auto kSmallTimeout = std::chrono::milliseconds{50};

// Experimentally approved limit for ~3Gb RSS
constexpr auto kMaxParallelRequests = 10'000;

constexpr char kTestData[] = "Test Data";

struct Callback {
    HttpResponse operator()(const HttpRequest& request) {
        LOG_INFO() << "HTTP Server receive: " << request;

        engine::InterruptibleSleepFor(kSmallTimeout);

        return {
            fmt::format("HTTP/1.1 {} OK\r\nContent-Length: ", code) + "4096\r\n\r\n" + std::string(4096, '@'),
            HttpResponse::kWriteAndClose
        };
    }

    int code;
};

}  // namespace

template <typename Client>
void FireLoop(Client& client, const utest::SimpleServer& server)
{
    for (auto _ : utest::StressLoop()) {
        if (client.GetActiveRequestCountDebug() < kMaxParallelRequests) {
            for (unsigned i = 0; i < 500; ++i) {
                client.CreateRequest()
                    .post(server.GetBaseUrl(), kTestData)
                    .retry(3)
                    .timeout(kSmallTimeout)
                    .async_perform()
                    .Detach();  // Do not do like this in production code!
            }
        }
        engine::SleepFor(std::chrono::microseconds(1));
    }
}

UTEST(HttpClientStress, DISABLED_IN_ASAN_TEST_NAME(PostShutdownWithPendingRequestCore)) {
    const utest::SimpleServer http_server{Callback{200}};
    FireLoop(*utest::impl::CreateHttpClientCore(), http_server);
}

UTEST(HttpClientStress, DISABLED_IN_ASAN_TEST_NAME(PostShutdownWithPendingRequestCoreRetries)) {
    const utest::SimpleServer http_server{Callback{500}};
    FireLoop(*utest::impl::CreateHttpClientCore(), http_server);
}

UTEST(HttpClientStress, DISABLED_IN_ASAN_TEST_NAME(PostShutdownWithPendingRequestRetries)) {
    const utest::SimpleServer http_server{Callback{500}};
    FireLoop(*utest::impl::CreateHttpClientWithMiddlewares(), http_server);
}

USERVER_NAMESPACE_END
