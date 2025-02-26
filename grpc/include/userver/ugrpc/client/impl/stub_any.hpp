#pragma once

#include <grpcpp/channel.h>
#include <grpcpp/generic/generic_stub.h>

#include <userver/utils/any_movable.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

using StubAny = utils::AnyMovable;

template <typename Stub>
StubAny MakeStub(const std::shared_ptr<grpc::Channel>& channel) {
    if constexpr (std::is_same_v<grpc::GenericStub, Stub>) {
        return StubAny{std::in_place_type<grpc::GenericStub>, std::shared_ptr<grpc::Channel>{channel}};
    }
    return StubAny{std::in_place_type<Stub>, channel};
}

template <typename Stub>
Stub& StubCast(StubAny& stub) {
    return utils::AnyCast<Stub&>(stub);
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
