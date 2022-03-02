#pragma once

#include <chrono>
#include <optional>

#include <grpcpp/support/status.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

class MethodStatistics;

class RpcStatisticsScope final {
 public:
  explicit RpcStatisticsScope(MethodStatistics& statistics);

  ~RpcStatisticsScope();

  void OnExplicitFinish(grpc::StatusCode code);

  void OnNetworkError();

 private:
  // Represents how the RPC was finished. Kinds with higher numeric values
  // override those with lower ones.
  enum class FinishKind {
    // The user didn't finish the RPC explicitly (sometimes due to an
    // exception), which indicates an internal service error
    kAutomatic = 0,

    // The user has finished the RPC (with some status code)
    kExplicit = 1,

    // A network error occurred (RpcInterruptedError)
    kNetworkError = 2,
  };

  void AccountStatus();
  void AccountTiming();

  MethodStatistics& statistics_;
  std::optional<std::chrono::steady_clock::time_point> start_time_;
  FinishKind finish_kind_{FinishKind::kAutomatic};
  grpc::StatusCode finish_code_{};
};

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
