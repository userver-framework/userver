#pragma once

#include <grpcpp/server_builder.h>

#include <userver/ugrpc/impl/completion_queue_pool_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

/// @brief Manages a gRPC completion queue, usable in services and clients.
/// @note During shutdown, `QueueHolder`s must be destroyed after
/// `grpc::Server::Shutdown` is called, but before ugrpc::server::Reactor
/// instances are destroyed.
class CompletionQueuePool final : public ugrpc::impl::CompletionQueuePoolBase {
public:
    explicit CompletionQueuePool(std::size_t queue_count, grpc::ServerBuilder& server_builder);

    grpc::ServerCompletionQueue& GetQueue(std::size_t idx) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
        return static_cast<grpc::ServerCompletionQueue&>(CompletionQueuePoolBase::GetQueue(idx));
    }

    grpc::ServerCompletionQueue& NextQueue() {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
        return static_cast<grpc::ServerCompletionQueue&>(CompletionQueuePoolBase::NextQueue());
    }
};

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
