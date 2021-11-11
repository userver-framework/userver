#pragma once

#include <grpcpp/channel.h>
#include <grpcpp/security/credentials.h>

#include <userver/engine/task/task_processor_fwd.hpp>

/// @file userver/ugrpc/client/channels.hpp
/// @brief Utilities for managing gRPC connections

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

/// @brief Create a new gRPC channel (connection pool) for the endpoint
///
/// Channel creation is expensive, reuse channels if possible.
///
/// Each channel is fully thread-safe: it can be used across multiple threads,
/// possibly from multiple clients and using multiple `CompletionQueue`
/// instances at the same time.
///
/// @param blocking_task_processor task processor for blocking channel creation
/// @param channel_credentials channel credentials
/// @param endpoint string host:port
/// @returns shared pointer to the gRPC channel
std::shared_ptr<::grpc::Channel> MakeChannel(
    engine::TaskProcessor& blocking_task_processor,
    std::shared_ptr<::grpc::ChannelCredentials> channel_credentials,
    const std::string& endpoint);

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
