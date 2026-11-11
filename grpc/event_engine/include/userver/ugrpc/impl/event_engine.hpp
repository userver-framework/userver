#pragma once

#include <memory>

#include <grpc/event_engine/event_engine.h>
#include <grpcpp/server_builder.h>

#include <userver/engine/task/task_processor_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl::event_engine {

/// Whether the process-wide EventEngine runtime settings have been configured.
bool IsRuntimeConfigured() noexcept;

#if defined(ARCADIA_ROOT) && defined(__linux__)

/// Initialize process-wide settings before constructing gRPC objects.
void EnsureRuntimeConfigured();

class PosixEventEngine;

/// Owns and drains the shared server engine. The target task processor must outlive the owner.
class PosixEventEngineOwner final {
public:
    /// Construct from a coroutine so initialization failures can drain safely.
    explicit PosixEventEngineOwner(engine::TaskProcessor& task_processor);

    /// Destroy from a coroutine unless shutdown has already completed.
    ~PosixEventEngineOwner();

    PosixEventEngineOwner(const PosixEventEngineOwner&) = delete;
    PosixEventEngineOwner& operator=(const PosixEventEngineOwner&) = delete;

    /// Return the public gRPC interface without exposing the concrete engine implementation.
    std::shared_ptr<grpc_event_engine::experimental::EventEngine> GetEventEngine() const noexcept;

    /// Stop dependent servers and listeners first; call from one coroutine outside gRPC callbacks.
    /// Drains pending work. Repeated calls after completion are allowed.
    void Shutdown() noexcept;

private:
    std::shared_ptr<PosixEventEngine> engine_;
};

/// Construct after @ref ugrpc::impl::event_engine::EnsureRuntimeConfigured and grpc_init().
/// The callback CQ setup relies on gRPC's own `epoll1` I/O poller.
/// Retain the gRPC initialization reference until the owner finishes shutdown.
std::unique_ptr<PosixEventEngineOwner> CreateEventEngine(engine::TaskProcessor& task_processor);

/// @brief Register the argument adapter used by @ref ugrpc::impl::event_engine::SetEventEngine.
///
/// The bundled gRPC strips `grpc.internal.*` arguments before custom preconditioning stages.
/// This adapter converts `grpc.userver.event_engine` to gRPC's typed EventEngine argument after that
/// filtering, preserving shared ownership of the engine. Call before grpc_init() or constructing
/// any gRPC objects.
///
/// On gRPC upgrades, recheck argument filtering, stage order and shared ownership. Replace this
/// adapter when the public per-server EventEngine API becomes available.
void RegisterEventEngineArgument();

/// @brief Select a nonempty EventEngine for one server, leaving the process-wide default unchanged.
///
/// Provides the per-server setter described by upstream but absent from the bundled grpc::ServerBuilder.
/// The builder carries shared ownership through `grpc.userver.event_engine`; the adapter registered by
/// @ref ugrpc::impl::event_engine::RegisterEventEngineArgument converts it to gRPC's native argument.
///
/// Call @ref ugrpc::impl::event_engine::EnsureRuntimeConfigured and
/// @ref ugrpc::impl::event_engine::RegisterEventEngineArgument before gRPC initialization.
/// Call this function before grpc::ServerBuilder::BuildAndStart(). For a userver engine, stop and destroy
/// the server and builder before shutting down its owner. Keep the owner's task processor and EV threads
/// running until that shutdown completes.
///
/// @see gRPC's proposed per-server EventEngine API:
/// https://github.com/grpc/grpc/blob/b4dd253f3aba2076b171ebe3b9f639f514c034ab/include/grpc/event_engine/event_engine.h#L84
void SetEventEngine(
    grpc::ServerBuilder& builder,
    std::shared_ptr<grpc_event_engine::experimental::EventEngine> event_engine
);

#endif

}  // namespace ugrpc::impl::event_engine

USERVER_NAMESPACE_END
