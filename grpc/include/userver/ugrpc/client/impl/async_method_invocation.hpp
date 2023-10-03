#pragma once

#include <optional>

#include <google/rpc/status.pb.h>
#include <grpcpp/client_context.h>
#include <grpcpp/impl/codegen/status.h>

#include <userver/ugrpc/impl/async_method_invocation.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

class RpcData;

/// @brief Contains parsed additional data for grpc status
/// For example parsed status string
struct ParsedGStatus final {
  /// @brief Processes status and builds ParsedGStatus
  static ParsedGStatus ProcessStatus(const grpc::Status& status);

  std::optional<google::rpc::Status> gstatus;
  std::optional<std::string> gstatus_string;
};

/// AsyncMethodInvocation for Finish method that stops stats and Span timers
/// ASAP, without waiting for a Task to wake up
class FinishAsyncMethodInvocation final
    : public ugrpc::impl::AsyncMethodInvocation {
 public:
  explicit FinishAsyncMethodInvocation(RpcData& rpc_data);
  ~FinishAsyncMethodInvocation() override;

  grpc::Status& GetStatus();
  ParsedGStatus& GetParsedGStatus();

  void Notify(bool ok) noexcept override;

 private:
  RpcData& rpc_data_;
  grpc::Status& status_;
  ParsedGStatus parsed_gstatus_;
};

ugrpc::impl::AsyncMethodInvocation::WaitStatus Wait(
    ugrpc::impl::AsyncMethodInvocation& invocation,
    grpc::ClientContext& context) noexcept;

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
