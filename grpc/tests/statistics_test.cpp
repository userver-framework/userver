#include <userver/utest/utest.hpp>

#include <userver/engine/get_all.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/mock_now.hpp>

#include <userver/ugrpc/client/exceptions.hpp>

#include <tests/deadline_helpers.hpp>
#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>

USERVER_NAMESPACE_BEGIN

using namespace std::chrono_literals;

namespace {

constexpr auto kSayHelloSleepTime = std::chrono::milliseconds{20};

class UnitTestServiceForStatistics final : public sample::ugrpc::UnitTestServiceBase {
public:
    SayHelloResult SayHello(CallContext& /*context*/, sample::ugrpc::GreetingRequest&& /*request*/) override {
        engine::SleepFor(kSayHelloSleepTime);
        return grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "message", "details"};
    }

    ChatResult Chat(CallContext& /*context*/, ChatReaderWriter& /*stream*/) override {
        return grpc::Status{grpc::StatusCode::UNIMPLEMENTED, "message", "details"};
    }

    GetCustomStatusCodeResult GetCustomStatusCode(CallContext& /*context*/, ::google::protobuf::Empty&& /*request*/)
        override {
        return grpc::Status{static_cast<grpc::StatusCode>(4242), "message", "details"};
    }
};

}  // namespace

using GrpcStatistics = ugrpc::tests::ServiceFixture<UnitTestServiceForStatistics>;

UTEST_F(GrpcStatistics, LongRequest) {
    auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
    sample::ugrpc::GreetingRequest out;
    out.set_name("userver");
    UEXPECT_THROW(client.SayHello(out), ugrpc::client::InvalidArgumentError);
    GetServer().StopServing();

    for (const auto& domain : {"client", "server"}) {
        const auto stats = GetStatistics(
            fmt::format("grpc.{}.by-destination", domain),
            {{"grpc_destination", "sample.ugrpc.UnitTestService/SayHello"}}
        );

        const auto get_status_code_count = [&](const std::string& code) {
            const auto metric_optional = stats.SingleMetricOptional("status", {{"grpc_code", code}});
            return metric_optional ? std::make_optional(metric_optional->AsRate()) : std::nullopt;
        };

        EXPECT_EQ(get_status_code_count("OK"), 0);
        EXPECT_EQ(get_status_code_count("INVALID_ARGUMENT"), 1);
        // Because there have been no RPCs with that status.
        EXPECT_EQ(get_status_code_count("ALREADY_EXISTS"), std::nullopt);
        EXPECT_EQ(stats.SingleMetric("rps").AsRate(), 1);
        EXPECT_EQ(stats.SingleMetric("eps").AsRate(), 0);
        EXPECT_EQ(stats.SingleMetric("network-error").AsRate(), 0);
        EXPECT_EQ(stats.SingleMetric("abandoned-error").AsRate(), 0);
    }
}

UTEST_F(GrpcStatistics, DelayBeforeGet) {
    // Consider a situation where response futures are stockpiled
    // somewhere, and the task awaits something else (more responses?) before
    // calling response futures Get. In this case, metrics should still contain timings
    // the response is actually received on the network.

    utils::datetime::MockNowSet({});

    auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
    sample::ugrpc::GreetingRequest out;
    out.set_name("userver");

    auto future = client.AsyncSayHello(out);

    const std::string kMetricsPath = "grpc.client.by-destination";
    const std::vector<utils::statistics::Label> kMetricsLabels{
        {"grpc_destination", "sample.ugrpc.UnitTestService/SayHello"},
    };

    const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);
    while (!future.IsReady()) {
        if (deadline.IsReached()) {
            FAIL() << "Response could not reach Client until max test timeout";
        }
        engine::InterruptibleSleepFor(std::chrono::milliseconds{10});
    }

    const auto delay_before_get = tests::kLongTimeout;
    engine::InterruptibleSleepFor(delay_before_get);

    UEXPECT_THROW(future.Get(), ugrpc::client::InvalidArgumentError);

    // So that RecentPeriod "timings" metric makes the current epoch readable.
    utils::datetime::MockSleep(6s);

    const auto stats = GetStatistics(kMetricsPath, kMetricsLabels);

    // check status
    EXPECT_EQ(stats.SingleMetric("status", {{"grpc_code", "INVALID_ARGUMENT"}}).AsRate(), 1);
    // check rps
    EXPECT_EQ(stats.SingleMetric("rps").AsRate(), 1);

    // check timings
    const auto timing_ms = stats.SingleMetric("timings", {{"percentile", "p100"}}).AsInt();
    EXPECT_GE(timing_ms, std::chrono::duration_cast<std::chrono::milliseconds>(kSayHelloSleepTime).count());
    EXPECT_LT(timing_ms, std::chrono::duration_cast<std::chrono::milliseconds>(delay_before_get).count());
}

UTEST_F_MT(GrpcStatistics, Multithreaded, 2) {
    constexpr int kIterations = 10;

    auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();

    auto say_hello_task = utils::Async("say-hello", [&] {
        for (int i = 0; i < kIterations; ++i) {
            sample::ugrpc::GreetingRequest out;
            out.set_name("userver");
            UEXPECT_THROW(client.SayHello(out), ugrpc::client::InvalidArgumentError);
        }
    });

    auto chat_task = utils::Async("chat", [&] {
        for (int i = 0; i < kIterations; ++i) {
            auto chat = client.Chat();
            sample::ugrpc::StreamGreetingResponse response;
            UEXPECT_THROW((void)chat.Read(response), ugrpc::client::UnimplementedError);
        }
    });

    engine::GetAll(say_hello_task, chat_task);
    GetServer().StopServing();

    for (const auto& domain : {"client", "server"}) {
        const auto status = GetStatistics(fmt::format("grpc.{}.by-destination.status", domain));
        const utils::statistics::Label say_hello_label{"grpc_destination", "sample.ugrpc.UnitTestService/SayHello"};
        const utils::statistics::Label chat_label{"grpc_destination", "sample.ugrpc.UnitTestService/Chat"};

        const auto get_status_code_count = [&](const auto& label, auto code) {
            const auto metric_optional = status.SingleMetricOptional("", {label, {"grpc_code", code}});
            return metric_optional ? std::make_optional(metric_optional->AsRate()) : std::nullopt;
        };

        EXPECT_EQ(get_status_code_count(say_hello_label, "INVALID_ARGUMENT"), kIterations);
        EXPECT_EQ(get_status_code_count(say_hello_label, "OK"), 0);
        // Because there have been no RPCs with that status.
        EXPECT_EQ(get_status_code_count(say_hello_label, "UNIMPLEMENTED"), std::nullopt);

        EXPECT_EQ(get_status_code_count(chat_label, "UNIMPLEMENTED"), kIterations);
        EXPECT_EQ(get_status_code_count(chat_label, "OK"), 0);
        // Because there have been no RPCs with that status.
        EXPECT_EQ(get_status_code_count(chat_label, "INVALID_ARGUMENT"), std::nullopt);
    }
}

#if defined(__has_feature) && defined(__clang__)
#if !__has_feature(undefined_behavior_sanitizer)
// at the moment, codes out of range are UB. See https://github.com/grpc/grpc/issues/38882
UTEST_F(GrpcStatistics, CustomCodes) {
    auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
    UEXPECT_THROW(client.GetCustomStatusCode(google::protobuf::Empty{}), ugrpc::client::UnknownError);
    GetServer().StopServing();

    for (const auto& domain : {"client", "server"}) {
        const auto stats = GetStatistics(
            fmt::format("grpc.{}.by-destination", domain),
            {{"grpc_destination", "sample.ugrpc.UnitTestService/GetCustomStatusCode"}}
        );

        const auto get_status_code_count = [&](const std::string& code) {
            const auto metric_optional = stats.SingleMetricOptional("status", {{"grpc_code", code}});
            return metric_optional ? std::make_optional(metric_optional->AsRate()) : std::nullopt;
        };

        EXPECT_EQ(get_status_code_count("OK"), 0);
        EXPECT_EQ(get_status_code_count("non_standard"), 1);
        EXPECT_EQ(stats.SingleMetric("rps").AsRate(), 1);
        EXPECT_EQ(stats.SingleMetric("eps").AsRate(), 1);
    }
}
#endif
#endif

USERVER_NAMESPACE_END
