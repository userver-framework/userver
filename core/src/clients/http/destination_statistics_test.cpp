#include <clients/http/destination_statistics.hpp>

#include <unordered_set>

#include <userver/engine/sleep.hpp>
#include <userver/logging/log.hpp>

#include <userver/clients/http/client_core.hpp>
#include <userver/server/request/client_metrics_shard.hpp>
#include <userver/utest/http_client.hpp>
#include <userver/utest/simple_server.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/statistics/testing.hpp>

USERVER_NAMESPACE_BEGIN

using HttpResponse = utest::SimpleServer::Response;
using HttpRequest = utest::SimpleServer::Request;

static HttpResponse Callback(int code, const HttpRequest& request) {
    LOG_INFO() << "HTTP Server receive: " << request;

    return {
        "HTTP/1.1 " + std::to_string(code) +
            " OK\r\nConnection: close\r\rContent-Length: "
            "0\r\n\r\n",
        HttpResponse::kWriteAndClose
    };
}

UTEST(DestinationStatistics, Empty) {
    auto client = utest::impl::CreateHttpClientCore();

    client->GetDestinationStatistics()
        .VisitAllDebug([](const clients::http::DestinationLabels&, const clients::http::Statistics&) {
            ADD_FAILURE() << "Should be empty";
        });
}

UTEST(DestinationStatistics, Ok) {
    const utest::SimpleServer http_server{[](const HttpRequest& request) { return Callback(200, request); }};
    auto client = utest::impl::CreateHttpClientCore();

    auto url = http_server.GetBaseUrl();

    auto response = client->CreateRequest().post(url).retry(1).timeout(std::chrono::milliseconds(100)).perform();

    size_t size = 0;

    const auto visitor = [&](const clients::http::DestinationLabels& labels, const clients::http::Statistics& stat) {
        ASSERT_EQ(1, ++size);

        EXPECT_EQ(url, *labels.http_destination);

        auto stats = clients::http::InstanceStatistics(stat);
        auto ok = static_cast<size_t>(clients::http::Statistics::ErrorGroup::kOk);
        EXPECT_EQ(utils::statistics::Rate{1}, stats.error_count[ok]);
        for (size_t i = 0; i < clients::http::Statistics::kErrorGroupCount; i++) {
            if (i != ok) {
                EXPECT_EQ(utils::statistics::Rate{0}, stats.error_count[i]);
            }
        }
    };
    client->GetDestinationStatistics().VisitAllDebug(visitor);
}

UTEST(DestinationStatistics, CancelledFuture) {
    const utest::SimpleServer http_server{[](const HttpRequest& request) {
        engine::InterruptibleSleepFor(utest::kMaxTestWaitTime);
        return Callback(200, request);
    }};
    auto client = utest::impl::CreateHttpClientCore();

    auto url = http_server.GetBaseUrl();

    {
        auto response_future =
            client->CreateRequest().post(url).retry(1).timeout(std::chrono::milliseconds(100)).async_perform();
    }

    const auto& pool_stats = client->GetPoolStatistics();
    EXPECT_EQ(pool_stats.multi.size(), 1);
    EXPECT_EQ(
        pool_stats.multi[0].error_count[static_cast<size_t>(clients::http::Statistics::ErrorGroup::kUnknown)],
        utils::statistics::Rate{0}
    );
    EXPECT_EQ(
        pool_stats.multi[0].error_count[static_cast<size_t>(clients::http::Statistics::ErrorGroup::kCancelled)],
        utils::statistics::Rate{1}
    );
}

UTEST(DestinationStatistics, Multiple) {
    const utest::SimpleServer http_server{[](const HttpRequest& request) { return Callback(200, request); }};
    const utest::SimpleServer http_server2{[](const HttpRequest& request) { return Callback(500, request); }};
    auto client = utest::impl::CreateHttpClientCore();

    client->SetDestinationMetricsAutoMaxSize(100);

    auto url = http_server.GetBaseUrl();
    auto url2 = http_server2.GetBaseUrl();

    auto response = client->CreateRequest().post(url).retry(1).timeout(std::chrono::milliseconds(100)).perform();
    response = client->CreateRequest().post(url2).retry(1).timeout(std::chrono::milliseconds(100)).perform();

    size_t size = 0;
    std::unordered_set<std::string> expected_urls{url, url2};
    const auto visitor = [&](const clients::http::DestinationLabels& labels, const clients::http::Statistics& stat) {
        ASSERT_LE(++size, 2);

        EXPECT_EQ(1, expected_urls.erase(std::string{labels.http_destination}));

        auto stats = clients::http::InstanceStatistics(stat);
        auto ok = static_cast<size_t>(clients::http::Statistics::ErrorGroup::kOk);
        EXPECT_EQ(utils::statistics::Rate{1}, stats.error_count[ok]);
        for (size_t i = 0; i < clients::http::Statistics::kErrorGroupCount; i++) {
            if (i != ok) {
                EXPECT_EQ(utils::statistics::Rate{0}, stats.error_count[i]) << i << " errors must be zero";
            }
        }
    };
    client->GetDestinationStatistics().VisitAllDebug(visitor);
}

UTEST(DestinationStatistics, Sharded) {
    using utils::statistics::Rate;

    const utest::SimpleServer http_server{[](const HttpRequest& request) { return Callback(200, request); }};
    auto client = utest::impl::CreateHttpClientCore();
    client->SetDestinationMetricsAutoMaxSize(100);

    const auto url = http_server.GetBaseUrl();
    const auto perform = [&client, &url] {
        return client->CreateRequest().post(url).retry(1).timeout(std::chrono::milliseconds(100)).perform();
    };

    EXPECT_EQ(perform()->status_code(), 200);

    server::request::SetClientMetricsShard("by_platform", {{"platform", "desktop"}});
    EXPECT_EQ(perform()->status_code(), 200);
    EXPECT_EQ(utils::Async("child", perform).Get()->status_code(), 200);

    const utils::statistics::Snapshot snapshot{client->GetDestinationStatistics()};

    EXPECT_EQ(snapshot.SingleMetric("errors", {{"http_destination", url}, {"http_error", "ok"}}), Rate{3});

    const std::vector<utils::statistics::Label> shard_labels{{"http_destination", url}, {"platform", "desktop"}};
    auto with_shard_labels = [&shard_labels](utils::statistics::Label label) {
        auto labels = shard_labels;
        labels.push_back(std::move(label));
        return labels;
    };
    EXPECT_EQ(snapshot.SingleMetric("by_platform.errors", with_shard_labels({"http_error", "ok"})), Rate{2});
    EXPECT_EQ(snapshot.SingleMetric("by_platform.reply-statuses", with_shard_labels({"http_code", "200"})), Rate{2});
    EXPECT_TRUE(snapshot.SingleMetricOptional("by_platform.timings", with_shard_labels({"percentile", "p50"})));
    EXPECT_FALSE(snapshot.SingleMetricOptional("by_platform.retries", shard_labels));
}

UTEST(DestinationStatistics, ShardedExplicitDestination) {
    using utils::statistics::Rate;

    const utest::SimpleServer http_server{[](const HttpRequest& request) { return Callback(200, request); }};
    auto client = utest::impl::CreateHttpClientCore();

    server::request::SetClientMetricsShard("by_platform", {{"platform", "desktop"}});
    const auto response =
        client->CreateRequest()
            .post(http_server.GetBaseUrl())
            .SetDestinationMetricName("explicit-destination")
            .retry(1)
            .timeout(std::chrono::milliseconds(100))
            .perform();
    EXPECT_EQ(response->status_code(), 200);

    const utils::statistics::Snapshot snapshot{client->GetDestinationStatistics()};
    EXPECT_EQ(
        snapshot.SingleMetric(
            "by_platform.errors",
            {{"http_destination", "explicit-destination"}, {"http_error", "ok"}, {"platform", "desktop"}}
        ),
        Rate{1}
    );
}

UTEST(DestinationStatistics, ShardedErase) {
    using utils::statistics::Rate;

    const utest::SimpleServer http_server{[](const HttpRequest& request) { return Callback(200, request); }};
    auto client = utest::impl::CreateHttpClientCore();
    client->SetDestinationMetricsAutoMaxSize(100);

    const auto url = http_server.GetBaseUrl();
    const auto perform = [&client, &url] {
        return client->CreateRequest().post(url).retry(1).timeout(std::chrono::milliseconds(100)).perform();
    };

    server::request::SetClientMetricsShard("by_platform", {{"platform", "desktop"}});
    EXPECT_EQ(perform()->status_code(), 200);

    server::request::EraseClientMetricsShard();
    EXPECT_EQ(perform()->status_code(), 200);

    const utils::statistics::Snapshot snapshot{client->GetDestinationStatistics()};
    EXPECT_EQ(snapshot.SingleMetric("errors", {{"http_destination", url}, {"http_error", "ok"}}), Rate{2});
    EXPECT_EQ(snapshot.SingleMetric("by_platform.errors", {{"http_destination", url}, {"http_error", "ok"}}), Rate{1});
}

UTEST(DestinationStatistics, ShardedScope) {
    using utils::statistics::Rate;

    const utest::SimpleServer http_server{[](const HttpRequest& request) { return Callback(200, request); }};
    auto client = utest::impl::CreateHttpClientCore();
    client->SetDestinationMetricsAutoMaxSize(100);

    const auto url = http_server.GetBaseUrl();
    const auto perform = [&client, &url] {
        return client->CreateRequest().post(url).retry(1).timeout(std::chrono::milliseconds(100)).perform();
    };

    {
        const server::request::ClientMetricsShardScope desktop_scope{"by_platform", {{"platform", "desktop"}}};
        {
            const server::request::ClientMetricsShardScope touch_scope{"by_platform", {{"platform", "touch"}}};
            EXPECT_EQ(perform()->status_code(), 200);
        }
        EXPECT_EQ(perform()->status_code(), 200);
    }
    EXPECT_EQ(perform()->status_code(), 200);

    const utils::statistics::Snapshot snapshot{client->GetDestinationStatistics()};
    EXPECT_EQ(snapshot.SingleMetric("errors", {{"http_destination", url}, {"http_error", "ok"}}), Rate{3});
    EXPECT_EQ(snapshot.SingleMetric("by_platform.errors", {{"http_error", "ok"}, {"platform", "touch"}}), Rate{1});
    EXPECT_EQ(snapshot.SingleMetric("by_platform.errors", {{"http_error", "ok"}, {"platform", "desktop"}}), Rate{1});
}

USERVER_NAMESPACE_END
