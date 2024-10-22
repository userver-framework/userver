#include <userver/utest/utest.hpp>

#include <userver/engine/get_all.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/mock_now.hpp>

#include <userver/ugrpc/client/exceptions.hpp>

#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>

USERVER_NAMESPACE_BEGIN

using namespace std::chrono_literals;

namespace {

class UnitTestServiceForStatistics final : public sample::ugrpc::UnitTestServiceBase {
public:
    void SayHello(SayHelloCall& call, sample::ugrpc::GreetingRequest&&) override {
        engine::SleepFor(std::chrono::milliseconds{20});
        call.FinishWithError({grpc::StatusCode::INVALID_ARGUMENT, "message", "details"});
    }

    void Chat(ChatCall& call) override {
        call.FinishWithError({grpc::StatusCode::UNIMPLEMENTED, "message", "details"});
    }
};

}  // namespace

using GrpcStatistics = ugrpc::tests::ServiceFixture<UnitTestServiceForStatistics>;

UTEST_F(GrpcStatistics, LongRequest) {
    auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
    sample::ugrpc::GreetingRequest out;
    out.set_name("userver");
    UEXPECT_THROW(client.SayHello(out).Finish(), ugrpc::client::InvalidArgumentError);
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

UTEST_F(GrpcStatistics, StatsBeforeGet) {
    // In this test, we ensure that stats are accounted for even if we don't call
    // future.Get(). Consider a situation where such futures are stockpiled
    // somewhere, and the task awaits something else (more responses?) before
    // calling Get. In this case, metrics should still be written as soon as
    // the response is actually received on the network.

    utils::datetime::MockNowSet({});

    auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
    sample::ugrpc::GreetingRequest out;
    sample::ugrpc::GreetingResponse res;
    out.set_name("userver");

    auto call = client.SayHello(out);
    auto future = call.FinishAsync(res);

    const std::string kMetricsPath = "grpc.client.by-destination";
    const std::vector<utils::statistics::Label> kMetricsLabels{
        {"grpc_destination", "sample.ugrpc.UnitTestService/SayHello"},
    };

    // Here we intend to wait until the client finishes processing the request and
    // updates the metrics asynchronously without actually calling Get. Pretty
    // much the only guaranteed way to await this is to wait until the metrics
    // arrive.
    const auto test_deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);
    while (true) {
        if (test_deadline.IsReached()) {
            FAIL() << "Client failed to set metrics until max test time";
        }

        if (GetStatistics(kMetricsPath, kMetricsLabels).SingleMetric("rps").AsRate() >= utils::statistics::Rate{1}) {
            break;
        }

        engine::SleepFor(std::chrono::milliseconds{1});
    }

    // So that RecentPeriod "timings" metric makes the current epoch readable.
    utils::datetime::MockSleep(6s);

    const auto stats = GetStatistics(kMetricsPath, kMetricsLabels);

    // check status
    EXPECT_EQ(stats.SingleMetric("status", {{"grpc_code", "INVALID_ARGUMENT"}}).AsRate(), 1);
    // check rps
    EXPECT_EQ(stats.SingleMetric("rps").AsRate(), 1);

    // check timings
    auto timing = stats.SingleMetric("timings", {{"percentile", "p100"}}).AsInt();
    EXPECT_GE(timing, 20);
    EXPECT_LT(timing, std::chrono::milliseconds{utest::kMaxTestWaitTime}.count());

    UEXPECT_THROW(future.Get(), ugrpc::client::InvalidArgumentError);
}

UTEST_F_MT(GrpcStatistics, Multithreaded, 2) {
    constexpr int kIterations = 10;

    auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();

    auto say_hello_task = utils::Async("say-hello", [&] {
        for (int i = 0; i < kIterations; ++i) {
            sample::ugrpc::GreetingRequest out;
            out.set_name("userver");
            UEXPECT_THROW(client.SayHello(out).Finish(), ugrpc::client::InvalidArgumentError);
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

USERVER_NAMESPACE_END
