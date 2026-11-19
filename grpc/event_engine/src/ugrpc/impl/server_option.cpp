#include <ugrpc/impl/server_option.hpp>

#include <memory>
#include <utility>
#include <vector>

#include <grpc/event_engine/event_engine.h>
#include <grpcpp/support/channel_arguments.h>
#include <grpcpp/support/config.h>
#include <src/core/lib/channel/channel_args.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl::event_engine {

ServerOption::ServerOption(std::shared_ptr<grpc_event_engine::experimental::EventEngine> event_engine)
    : event_engine_(std::move(event_engine))
{}

void ServerOption::UpdateArguments(grpc::ChannelArguments* args) {
    args->SetPointerWithVtable(
        grpc::string{kEventEngineArg},
        &event_engine_,
        grpc_core::ChannelArgTypeTraits<std::shared_ptr<grpc_event_engine::experimental::EventEngine>>::VTable()
    );
}

void ServerOption::UpdatePlugins(std::vector<std::unique_ptr<grpc::ServerBuilderPlugin>>*) {}

}  // namespace ugrpc::impl::event_engine

USERVER_NAMESPACE_END
