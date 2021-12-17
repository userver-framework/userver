#include <userver/ugrpc/client/client_factory.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

ClientFactory::ClientFactory(engine::TaskProcessor& channel_task_processor,
                             grpc::CompletionQueue& queue)
    : channel_task_processor_(channel_task_processor), queue_(queue) {}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
