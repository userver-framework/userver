#pragma once

#include <memory>

#include <grpc/event_engine/event_engine.h>
#include <grpcpp/server_builder.h>

#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/ugrpc/impl/grpc_init.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl::event_engine {

class PosixEventEngine;

// Owns and drains the shared server engine. The target task processor must outlive the owner.
//
// Configures the process-wide runtime and keeps gRPC initialized while it exists.
// Available only in Arcadia Linux builds.
// Construct the first owner or a GrpcInit before gRPC initialization or experiment lookup.
class PosixEventEngineOwner final {
public:
    // Construct from a coroutine so initialization failures can drain safely.
    // Throws std::runtime_error if the required gRPC experiments conflict with the environment.
    // An incompatible gRPC I/O poller fails UINVARIANT.
    explicit PosixEventEngineOwner(engine::TaskProcessor& task_processor);

    // Destroy from a coroutine unless shutdown has already completed.
    ~PosixEventEngineOwner();

    PosixEventEngineOwner(const PosixEventEngineOwner&) = delete;
    PosixEventEngineOwner& operator=(const PosixEventEngineOwner&) = delete;

    // Return the public gRPC interface without exposing the concrete engine implementation.
    std::shared_ptr<grpc_event_engine::experimental::EventEngine> GetEventEngine() const noexcept;

    // Stop dependent servers and listeners first; call from one coroutine outside gRPC callbacks.
    // Drains pending work. Repeated calls after completion are allowed.
    void Shutdown() noexcept;

private:
    // Keep gRPC initialized until the engine is released.
    GrpcInit grpc_init_;
    std::shared_ptr<PosixEventEngine> engine_;
};

// Create an engine and its owner, see PosixEventEngineOwner for the requirements.
// The callback CQ setup relies on gRPC's own `epoll1` I/O poller.
std::unique_ptr<PosixEventEngineOwner> CreateEventEngineOwner(engine::TaskProcessor& task_processor);

// Select a nonempty EventEngine for one server, leaving the process-wide default unchanged.
//
// Provides the per-server setter described by upstream but absent from the bundled grpc::ServerBuilder.
// The builder carries shared ownership through `grpc.userver.event_engine`; runtime initialization
// registers an adapter that converts it to gRPC's native argument.
//
// Create a PosixEventEngineOwner or GrpcInit to prepare the runtime before using this function.
// Call this function before grpc::ServerBuilder::BuildAndStart(). For a userver engine, stop and destroy
// the server and builder before shutting down its owner. Keep the owner's task processor and EV threads
// running until that shutdown completes.
//
// gRPC's proposed per-server EventEngine API:
// https://github.com/grpc/grpc/blob/b4dd253f3aba2076b171ebe3b9f639f514c034ab/include/grpc/event_engine/event_engine.h#L84
void SetEventEngine(
    grpc::ServerBuilder& builder,
    std::shared_ptr<grpc_event_engine::experimental::EventEngine> event_engine
);

}  // namespace ugrpc::impl::event_engine

USERVER_NAMESPACE_END
