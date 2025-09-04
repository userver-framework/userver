#pragma once

#include <chrono>
#include <string>

#include <userver/engine/deadline.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/server/request/task_inherited_data.hpp>

#include <userver/ugrpc/client/call_options.hpp>

#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

// TODO move functions to cpp
namespace tests {

constexpr auto kShortTimeout = std::chrono::milliseconds{300};
constexpr auto kLongTimeout = std::chrono::milliseconds{500} + kShortTimeout;

constexpr auto kAddSleep = std::chrono::milliseconds{100};

const std::string kGrpcMethod = "grpc_method";

inline ugrpc::client::CallOptions MakeCallOptions(bool set_timeout) {
    ugrpc::client::CallOptions call_options;
    if (set_timeout) {
        call_options.SetTimeout(kLongTimeout);
    }
    return call_options;
}

inline void InitTaskInheritedDeadline(const engine::Deadline deadline = engine::Deadline::FromDuration(kShortTimeout)) {
    server::request::kTaskInheritedData.Set({{}, kGrpcMethod, std::chrono::steady_clock::now(), deadline});
}

inline void WaitUntilRpcDeadlineService() { engine::InterruptibleSleepFor(utest::kMaxTestWaitTime); }

inline void WaitUntilRpcDeadlineClient(engine::Deadline deadline) {
    engine::SleepUntil(deadline);
    // kAddSleep is needed, because otherwise the background timer from grpc-core
    // might not manage to cancel the ClientContext in time.
    engine::SleepFor(kAddSleep);
}

}  // namespace tests

USERVER_NAMESPACE_END
