#pragma once

#include <grpc/event_engine/event_engine.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

// std::unique_ptr<grpc_event_engine::experimental::EventEngine> CreateEventEngine(engine::TaskProcessor&
// task_processor);

void SetEventEngineFactory();

void ShutdownDefaultEventEngine();

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
