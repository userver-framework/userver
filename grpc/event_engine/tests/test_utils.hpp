#pragma once

#include <memory>

#include <ugrpc/impl/event_engine/engine.hpp>
#include <userver/ugrpc/impl/event_engine.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl::event_engine {

inline std::shared_ptr<PosixEventEngine> GetPosixEventEngine(const PosixEventEngineOwner& owner) {
    return std::static_pointer_cast<PosixEventEngine>(owner.GetEventEngine());
}

}  // namespace ugrpc::impl::event_engine

USERVER_NAMESPACE_END
