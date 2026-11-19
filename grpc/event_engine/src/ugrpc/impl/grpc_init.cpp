#include <userver/ugrpc/impl/grpc_init.hpp>

#include <stdexcept>
#include <utility>

#include <grpc/event_engine/event_engine.h>
#include <grpc/grpc.h>
#include <src/core/lib/channel/channel_args.h>
#include <src/core/lib/config/core_configuration.h>
#include <src/core/lib/event_engine/shim.h>
#include <src/core/lib/experiments/config.h>
#include <src/core/lib/experiments/experiments.h>
#include <src/core/lib/iomgr/ev_epoll1_linux.h>
#include <src/core/lib/iomgr/ev_posix.h>

#include <ugrpc/impl/server_option.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl::event_engine {

namespace {

void RegisterEventEngineFactory() {
    static const auto kEpoll = [] {
        auto poller = grpc_ev_epoll1_posix;
        // In the bundled gRPC, this flag selects GRPC_CQ_CALLBACK instead of
        // CallbackAlternativeCQ and its polling threads. Our EventEngine provides
        // server I/O progress. Client CQs must remain polled: the flag also disables
        // client backup polling.
        //
        // On each gRPC upgrade, recheck all consumers of run_in_background and test
        // server/client progress and shutdown. Remove this override when gRPC selects
        // GRPC_CQ_CALLBACK for our EventEngine without it.
        poller.run_in_background = true;
        return poller;
    }();
    grpc_register_event_engine_factory(&kEpoll, true);
}

// Carry the server's shared_ptr<EventEngine> through public ServerBuilder args.
// The bundled gRPC strips grpc.internal.* before custom preconditioning stages.
// Convert this key to the native EventEngine argument after filtering, without
// replacing the global default engine. Register exactly once, before gRPC
// constructs its core configuration.
//
// On gRPC upgrades, recheck argument filtering, stage order and shared ownership.
// Replace this adapter when a public per-server EventEngine API is available.
void RegisterEventEngineChannelArg() {
    grpc_core::CoreConfiguration::RegisterBuilder([](grpc_core::CoreConfiguration::Builder* builder) {
        builder->channel_args_preconditioning()->RegisterStage([](grpc_core::ChannelArgs args) {
            if (!args.Contains(kEventEngineArg)) {
                return args;
            }
            const auto* event_engine_arg = args.GetPointer<grpc_event_engine::experimental::EventEngine>(kEventEngineArg
            );
            if (!event_engine_arg || !*event_engine_arg) {
                throw std::runtime_error("grpc.userver.event_engine is reserved for grpc-event-engine");
            }
            auto event_engine = *event_engine_arg;
            return args.Remove(kEventEngineArg)
                .SetObject<grpc_event_engine::experimental::EventEngine>(std::move(event_engine));
        });
    });
}

void ConfigureRuntime() {
    grpc_core::ForceEnableExperiment("event_engine_listener", true);
    grpc_core::ForceEnableExperiment("event_engine_client", false);
    grpc_core::ForceEnableExperiment("event_engine_dns", false);

    RegisterEventEngineFactory();
    RegisterEventEngineChannelArg();

    if (!grpc_event_engine::experimental::UseEventEngineListener() ||
        grpc_event_engine::experimental::UseEventEngineClient() || grpc_core::IsEventEngineDnsEnabled())
    {
        throw std::runtime_error(
            "The userver EventEngine requires event_engine_listener with event_engine_client and "
            "event_engine_dns disabled; remove conflicting GRPC_EXPERIMENTS overrides"
        );
    }
}

void EnsureRuntimeConfigured() {
    [[maybe_unused]] static const bool kConfigured = [] {
        ConfigureRuntime();
        return true;
    }();
}

}  // namespace

GrpcInit::GrpcInit() {
    EnsureRuntimeConfigured();
    grpc_init();
}

GrpcInit::~GrpcInit() { grpc_shutdown(); }

}  // namespace ugrpc::impl::event_engine

USERVER_NAMESPACE_END
