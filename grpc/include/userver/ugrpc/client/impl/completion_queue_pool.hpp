#pragma once

#include <userver/ugrpc/impl/completion_queue_pool_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

/// @brief Manages a gRPC completion queue, usable only in clients
class CompletionQueuePool final : public ugrpc::impl::CompletionQueuePoolBase {
public:
    explicit CompletionQueuePool(std::size_t queue_count);
};

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
