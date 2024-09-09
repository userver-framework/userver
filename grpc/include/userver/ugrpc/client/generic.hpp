#pragma once

/// @file userver/ugrpc/client/generic.hpp
/// @brief @copybrief ugrpc::client::GenericClient

#include <optional>
#include <string_view>

#include <grpcpp/client_context.h>
#include <grpcpp/support/byte_buffer.h>

#include <userver/ugrpc/client/impl/client_data.hpp>
#include <userver/ugrpc/client/qos.hpp>
#include <userver/ugrpc/client/rpc.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

struct GenericOptions {
  /// Client QOS for this call. Note that there is no QOS dynamic config by
  /// default, so unless a timeout is specified here, only the deadline
  /// propagation mechanism will affect the gRPC deadline.
  Qos qos{};

  /// If non-`nullopt`, metrics are accounted for specified fake call name.
  /// If `nullopt`, writes a set of metrics per real call name.
  /// If the microservice serves as a proxy and has untrusted clients, it is
  /// a good idea to have this option set to non-`nullopt` to avoid
  /// the situations where an upstream client can spam various RPCs with
  /// non-existent names, which leads to this microservice spamming RPCs
  /// with non-existent names, which leads to creating storage for infinite
  /// metrics and causes OOM.
  /// The default is to specify `"Generic/Generic"` fake call name.
  std::optional<std::string_view> metrics_call_name{"Generic/Generic"};
};

/// @ingroup userver_clients
///
/// @brief Allows to talk to gRPC services (generic and normal) using dynamic
/// method names.
///
/// Created using @ref ugrpc::client::ClientFactory::MakeClient.
///
/// `call_name` must be in the format `full.path.to.TheService/MethodName`.
/// Note that unlike in base grpc++, there must be no initial `/` character.
///
/// The API is mainly intended for proxies, where the request-response body is
/// passed unchanged, with settings taken solely from the RPC metadata.
/// In cases where the code needs to operate on the actual messages,
/// serialization of requests and responses is left as an excercise to the user.
///
/// Middlewares are customizable and are applied as usual, except that no
/// message hooks are called, meaning that there won't be any logs of messages
/// from the default middleware.
///
/// There are no per-call-name metrics by default,
/// for details see @ref GenericOptions::metrics_call_name.
///
/// ## Example GenericClient usage with known message types
///
/// @snippet grpc/tests/generic_client_test.cpp  sample
///
/// For a more complete sample, see @ref grpc_generic_api.
class GenericClient final {
 public:
  GenericClient(GenericClient&&) noexcept = default;
  GenericClient& operator=(GenericClient&&) noexcept = delete;

  /// Initiate a `single request -> single response` RPC with the given name.
  client::UnaryCall<grpc::ByteBuffer> UnaryCall(
      std::string_view call_name, const grpc::ByteBuffer& request,
      std::unique_ptr<grpc::ClientContext> context =
          std::make_unique<grpc::ClientContext>(),
      const GenericOptions& options = {}) const;

  /// @cond
  // For internal use only.
  explicit GenericClient(impl::ClientParams&&);
  /// @endcond

 private:
  template <typename Client>
  friend impl::ClientData& impl::GetClientData(Client& client);

  impl::ClientData impl_;
};

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
