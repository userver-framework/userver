#include <proxy_service.hpp>

#include <grpcpp/client_context.h>
#include <grpcpp/server_context.h>
#include <grpcpp/support/byte_buffer.h>

#include <userver/components/component.hpp>
#include <userver/ugrpc/byte_buffer_utils.hpp>
#include <userver/ugrpc/client/generic.hpp>
#include <userver/ugrpc/client/simple_client_component.hpp>

namespace samples {

namespace {

grpc::string ToGrpcString(grpc::string_ref str) {
  return {str.data(), str.size()};
}

void ProxyRequestMetadata(const grpc::ServerContext& server_context,
                          grpc::ClientContext& client_context) {
  // Proxy all client (request) metadata,
  // add some custom metadata as well.
  for (const auto& [key, value] : server_context.client_metadata()) {
    client_context.AddMetadata(ToGrpcString(key), ToGrpcString(value));
  }
  client_context.AddMetadata("proxy-name", "grpc-generic-proxy");
}

void ProxyTrailingResponseMetadata(const grpc::ClientContext& client_context,
                                   grpc::ServerContext& server_context) {
  // Proxy all server (response) trailing metadata,
  // add some custom metadata as well.
  for (const auto& [key, value] : client_context.GetServerTrailingMetadata()) {
    server_context.AddTrailingMetadata(ToGrpcString(key), ToGrpcString(value));
  }
  server_context.AddTrailingMetadata("proxy-name", "grpc-generic-proxy");
}

}  // namespace

ProxyService::ProxyService(const components::ComponentConfig& config,
                           const components::ComponentContext& context)
    : ugrpc::server::GenericServiceBase::Component(config, context),
      client_(context
                  .FindComponent<ugrpc::client::SimpleClientComponent<
                      ugrpc::client::GenericClient>>("generic-client")
                  .GetClient()) {}

void ProxyService::Handle(Call& call) {
  // In this example we proxy any unary RPC to client_, adding some metadata.

  // By default, generic service metrics are written with labels corresponding
  // to the fake 'Generic/Generic' call name.
  // In this example, we accept the OOM potential and store metrics per
  // the actual call name.
  // Read docs on ugrpc::server::GenericServiceBase for details.
  call.SetMetricsCallName(call.GetCallName());

  grpc::ByteBuffer request_bytes;
  // Read might throw on a broken RPC, just rethrow then.
  if (!call.Read(request_bytes)) {
    // The client has already called WritesDone.
    // We expect exactly 1 request, so that's an error for us.
    call.FinishWithError(grpc::Status{grpc::StatusCode::INVALID_ARGUMENT,
                                      "Expected exactly 1 request, given: 0"});
    return;
  }

  grpc::ByteBuffer ignored_request_bytes;
  // Wait until the client calls WritesDone before proceeding so that we know
  // that no misuse will occur later. For unary RPCs, clients will essentially
  // call WritesDone implicitly.
  if (call.Read(ignored_request_bytes)) {
    call.FinishWithError(
        grpc::Status{grpc::StatusCode::INVALID_ARGUMENT,
                     "Expected exactly 1 request, given: at least 2"});
    return;
  }

  auto client_context = std::make_unique<grpc::ClientContext>();
  ProxyRequestMetadata(call.GetContext(), *client_context);

  // Deadline propagation will work, as we've registered the DP middleware
  // in the config of grpc-server component.
  // Optionally, we can set an additional timeout using GenericOptions::qos.
  auto client_rpc = client_.UnaryCall(call.GetCallName(), request_bytes,
                                      std::move(client_context));

  grpc::ByteBuffer response_bytes;
  try {
    response_bytes = client_rpc.Finish();
  } catch (const ugrpc::client::ErrorWithStatus& ex) {
    // Proxy the error returned from client.
    ProxyTrailingResponseMetadata(client_rpc.GetContext(), call.GetContext());
    call.FinishWithError(ex.GetStatus());
    return;
  } catch (const ugrpc::client::RpcError& ex) {
    // Either the upstream client has cancelled our server RPC, or a network
    // failure has occurred, or the deadline has expired. See:
    // * ugrpc::client::RpcInterruptedError
    // * ugrpc::client::RpcCancelledError
    LOG_WARNING() << "Client RPC has failed: " << ex;
    call.FinishWithError(grpc::Status{grpc::StatusCode::UNAVAILABLE,
                                      "Failed to proxy the request"});
    return;
  }

  ProxyTrailingResponseMetadata(client_rpc.GetContext(), call.GetContext());

  // WriteAndFinish might throw on a broken RPC, just rethrow then.
  call.WriteAndFinish(response_bytes);
}

}  // namespace samples
