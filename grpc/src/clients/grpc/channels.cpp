#include <userver/clients/grpc/channels.hpp>

#include <grpcpp/create_channel.h>

#include <userver/engine/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::grpc {

std::shared_ptr<::grpc::Channel> MakeChannel(
    engine::TaskProcessor& blocking_task_processor,
    std::shared_ptr<::grpc::ChannelCredentials> channel_credentials,
    const std::string& endpoint) {
  // Spawn a blocking task creating a gRPC channel
  // This is third party code, no use of span inside it
  return engine::AsyncNoSpan(blocking_task_processor, ::grpc::CreateChannel,
                             std::ref(endpoint), std::ref(channel_credentials))
      .Get();
}

}  // namespace clients::grpc

USERVER_NAMESPACE_END
