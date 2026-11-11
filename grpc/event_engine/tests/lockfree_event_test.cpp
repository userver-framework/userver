#include <ugrpc/impl/event_engine/lockfree_event.hpp>

#include <atomic>
#include <barrier>
#include <thread>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl::event_engine {
namespace {

struct EventFixture {
    std::atomic<int> calls{0};
    y_absl::Status last_status;
    PosixEngineClosure closure{[this](y_absl::Status status) {
        last_status = std::move(status);
        ++calls;
    }};
    LockfreeEvent event;

    EventFixture() { event.InitEvent(); }
    ~EventFixture() {
        event.SetShutdown(y_absl::CancelledError("test cleanup"));
        event.DestroyEvent();
    }
};

TEST(GrpcEventEngineReadiness, ReadyBeforeAndAfterRegistration) {
    EventFixture f;
    f.event.SetReady();
    f.event.NotifyOn(&f.closure);
    EXPECT_EQ(f.calls, 1);
    EXPECT_TRUE(f.last_status.ok());
    f.event.NotifyOn(&f.closure);
    EXPECT_EQ(f.calls, 1);
    f.event.SetReady();
    EXPECT_EQ(f.calls, 2);
    EXPECT_TRUE(f.last_status.ok());
}

TEST(GrpcEventEngineReadiness, ShutdownCompletesPendingAndFutureWaitersWithTheFirstError) {
    EventFixture f;
    f.event.NotifyOn(&f.closure);
    const auto error = y_absl::UnavailableError("closed by owner");
    EXPECT_TRUE(f.event.SetShutdown(error));
    EXPECT_TRUE(f.event.IsShutdown());
    EXPECT_EQ(f.calls, 1);
    EXPECT_EQ(f.last_status, error);
    EXPECT_FALSE(f.event.SetShutdown(y_absl::CancelledError("second shutdown")));
    f.event.SetReady();
    EXPECT_EQ(f.calls, 1);
    f.event.NotifyOn(&f.closure);
    EXPECT_EQ(f.calls, 2);
    EXPECT_EQ(f.last_status, error);
}

TEST(GrpcEventEngineReadiness, RegistrationReadinessAndShutdownRaceCompleteExactlyOnce) {
    for (int iteration = 0; iteration < 256; ++iteration) {
        EventFixture f;
        std::barrier start{3};
        std::jthread notify([&] {
            start.arrive_and_wait();
            f.event.NotifyOn(&f.closure);
        });
        std::jthread ready([&] {
            start.arrive_and_wait();
            f.event.SetReady();
        });
        start.arrive_and_wait();
        EXPECT_TRUE(f.event.SetShutdown(y_absl::CancelledError("race shutdown")));
        notify.join();
        ready.join();
        EXPECT_EQ(f.calls, 1) << iteration;
        EXPECT_TRUE(f.last_status.ok() || y_absl::IsCancelled(f.last_status));
        EXPECT_TRUE(f.event.IsShutdown());
    }
}

}  // namespace
}  // namespace ugrpc::impl::event_engine

USERVER_NAMESPACE_END
