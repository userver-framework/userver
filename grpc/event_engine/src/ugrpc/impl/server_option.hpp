#pragma once

#include <memory>
#include <string_view>
#include <vector>

#include <grpc/event_engine/event_engine.h>
#include <grpcpp/impl/server_builder_option.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl::event_engine {

inline constexpr std::string_view kEventEngineArg = "grpc.userver.event_engine";

// Passes shared ownership of an EventEngine to ServerBuilder channel arguments.
class ServerOption final : public grpc::ServerBuilderOption {
public:
    explicit ServerOption(std::shared_ptr<grpc_event_engine::experimental::EventEngine> event_engine);

    void UpdateArguments(grpc::ChannelArguments* args) override;

    void UpdatePlugins(std::vector<std::unique_ptr<grpc::ServerBuilderPlugin>>* plugins) override;

private:
    std::shared_ptr<grpc_event_engine::experimental::EventEngine> event_engine_;
};

}  // namespace ugrpc::impl::event_engine

USERVER_NAMESPACE_END
