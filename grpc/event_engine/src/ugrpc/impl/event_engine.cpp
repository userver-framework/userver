#include <userver/ugrpc/impl/event_engine.hpp>

#if defined(ARCADIA_ROOT) && defined(__linux__)

#include <grpc/event_engine/event_engine.h>

#include <atomic>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#include <grpcpp/impl/server_builder_option.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/support/config.h>
#include <src/core/lib/channel/channel_args.h>
#include <src/core/lib/config/core_configuration.h>
#include <src/core/lib/event_engine/shim.h>
#include <src/core/lib/experiments/config.h>
#include <src/core/lib/experiments/experiments.h>
#include <src/core/lib/iomgr/ev_epoll1_linux.h>
#include <src/core/lib/iomgr/ev_posix.h>

#include <userver/utils/assert.hpp>

#include <ugrpc/impl/event_engine/engine.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl::event_engine {

namespace {

// Carry the server's shared_ptr<EventEngine> through public ServerBuilder args.
// The bundled gRPC strips grpc.internal.* before custom preconditioning stages.
// RegisterEventEngineArgument converts this key to the native EventEngine
// argument after filtering, without replacing the global default engine.
//
// On gRPC upgrades, recheck argument filtering, stage order and shared ownership.
// Replace this adapter when a public per-server EventEngine API is available.
constexpr std::string_view kEventEngineArg = "grpc.userver.event_engine";

std::atomic<bool> runtime_configured{false};

class ServerOption final : public grpc::ServerBuilderOption {
public:
    explicit ServerOption(std::shared_ptr<grpc_event_engine::experimental::EventEngine> event_engine)
        : event_engine_(std::move(event_engine))
    {}

    void UpdateArguments(grpc::ChannelArguments* args) override {
        args->SetPointerWithVtable(
            grpc::string{kEventEngineArg},
            &event_engine_,
            grpc_core::ChannelArgTypeTraits<std::shared_ptr<grpc_event_engine::experimental::EventEngine>>::VTable()
        );
    }

    void UpdatePlugins(std::vector<std::unique_ptr<grpc::ServerBuilderPlugin>>*) override {}

private:
    std::shared_ptr<grpc_event_engine::experimental::EventEngine> event_engine_;
};

}  // namespace

namespace {

void RegisterEventEngineFactory() {
    static const auto epoll = [] {
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
    grpc_register_event_engine_factory(&epoll, true);
}

void ConfigureRuntime() {
    grpc_core::ForceEnableExperiment("event_engine_listener", true);
    grpc_core::ForceEnableExperiment("event_engine_client", false);
    grpc_core::ForceEnableExperiment("event_engine_dns", false);

    RegisterEventEngineFactory();

    if (!grpc_event_engine::experimental::UseEventEngineListener() ||
        !grpc_event_engine::experimental::EventEngineSupportsFd() ||
        grpc_event_engine::experimental::UseEventEngineClient() || grpc_core::IsEventEngineDnsEnabled())
    {
        throw std::runtime_error(
            "grpc-event-engine requires event_engine_listener and POSIX FD support with event_engine_client and "
            "event_engine_dns disabled; remove conflicting GRPC_EXPERIMENTS overrides"
        );
    }
}

}  // namespace

bool IsRuntimeConfigured() noexcept { return runtime_configured.load(std::memory_order_acquire); }

void EnsureRuntimeConfigured() {
    static const bool configured = [] {
        ConfigureRuntime();
        return true;
    }();
    runtime_configured.store(configured, std::memory_order_release);
}

PosixEventEngineOwner::PosixEventEngineOwner(engine::TaskProcessor& task_processor)
    : engine_(std::make_shared<PosixEventEngine>(task_processor))
{}

PosixEventEngineOwner::~PosixEventEngineOwner() { Shutdown(); }

std::shared_ptr<grpc_event_engine::experimental::EventEngine> PosixEventEngineOwner::GetEventEngine() const noexcept {
    return engine_;
}

void PosixEventEngineOwner::Shutdown() noexcept { engine_->Shutdown(); }

std::unique_ptr<PosixEventEngineOwner> CreateEventEngine(engine::TaskProcessor& task_processor) {
    return std::make_unique<PosixEventEngineOwner>(task_processor);
}

void RegisterEventEngineArgument() {
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

void SetEventEngine(
    grpc::ServerBuilder& builder,
    std::shared_ptr<grpc_event_engine::experimental::EventEngine> event_engine
) {
    UASSERT(event_engine);
    builder.SetOption(std::make_unique<ServerOption>(std::move(event_engine)));
}

}  // namespace ugrpc::impl::event_engine

USERVER_NAMESPACE_END

#else

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl::event_engine {

bool IsRuntimeConfigured() noexcept { return false; }

}  // namespace ugrpc::impl::event_engine

USERVER_NAMESPACE_END

#endif
