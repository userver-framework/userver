#pragma once

#include <utility>

#include <userver/utils/any_movable.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

using StubAny = utils::AnyMovable;

template <typename Stub, typename... Args>
StubAny MakeStub(Args&&... args) {
    return StubAny{std::in_place_type<Stub>, std::forward<Args>(args)...};
}

template <typename Stub>
Stub& StubCast(StubAny& stub) {
    return utils::AnyCast<Stub&>(stub);
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
