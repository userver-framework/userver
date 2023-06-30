#pragma once

#include <chrono>
#include <memory>
#include <string>

#include <grpcpp/client_context.h>

#include <userver/engine/deadline.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/server/request/task_inherited_data.hpp>

USERVER_NAMESPACE_BEGIN

// TODO move functions to cpp
namespace helpers {

constexpr auto kShortTimeout = std::chrono::milliseconds{300};
constexpr auto kLongTimeout = std::chrono::milliseconds{500} + kShortTimeout;

constexpr auto kAddSleep = std::chrono::milliseconds{100};

const std::string kGrpcMethod = "grpc_method";

inline std::unique_ptr<grpc::ClientContext> GetContext(bool need_deadline) {
  auto context = std::make_unique<grpc::ClientContext>();
  if (need_deadline) {
    context->set_deadline(engine::Deadline::FromDuration(kLongTimeout));
  }
  return context;
}

inline void InitTaskInheritedDeadline(
    const engine::Deadline deadline =
        engine::Deadline::FromDuration(kShortTimeout)) {
  server::request::kTaskInheritedData.Set(
      {{}, kGrpcMethod, std::chrono::steady_clock::now(), deadline});
}

inline void ClearTaskInheritedDeadline() {
  server::request::kTaskInheritedData.Erase();
}

inline void WaitForDeadline(const engine::Deadline deadline) {
  if (!deadline.IsReached()) {
    engine::SleepUntil(deadline);
    // Some tests flacky without this... Should research this
    engine::SleepFor(kAddSleep);
  }
}

inline void WaitForDeadline(
    const std::chrono::system_clock::time_point deadline) {
  WaitForDeadline(engine::Deadline::FromTimePoint(deadline));
}

}  // namespace helpers

USERVER_NAMESPACE_END
