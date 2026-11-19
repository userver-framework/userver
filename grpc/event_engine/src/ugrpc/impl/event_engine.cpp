#include <userver/ugrpc/impl/event_engine.hpp>

#include <grpc/event_engine/event_engine.h>

#include <memory>
#include <utility>

#include <grpcpp/server_builder.h>
#include <src/core/lib/iomgr/ev_posix.h>

#include <userver/utils/assert.hpp>

#include <ugrpc/impl/event_engine/engine.hpp>
#include <ugrpc/impl/server_option.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl::event_engine {

PosixEventEngineOwner::PosixEventEngineOwner(engine::TaskProcessor& task_processor) {
    UINVARIANT(
        grpc_event_engine_run_in_background(),
        "The userver EventEngine requires gRPC's epoll1 I/O poller in background mode, but gRPC selected "
        "another poller; remove GRPC_POLL_STRATEGY or set it to epoll1"
    );
    engine_ = std::make_shared<PosixEventEngine>(task_processor);
}

PosixEventEngineOwner::~PosixEventEngineOwner() { Shutdown(); }

std::shared_ptr<grpc_event_engine::experimental::EventEngine> PosixEventEngineOwner::GetEventEngine() const noexcept {
    return engine_;
}

void PosixEventEngineOwner::Shutdown() noexcept { engine_->Shutdown(); }

std::unique_ptr<PosixEventEngineOwner> CreateEventEngineOwner(engine::TaskProcessor& task_processor) {
    return std::make_unique<PosixEventEngineOwner>(task_processor);
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
