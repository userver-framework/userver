#pragma once

#include <grpcpp/channel.h>

#include <userver/utils/fixed_array.hpp>

#include <userver/ugrpc/client/client_qos.hpp>
#include <userver/ugrpc/impl/stub_any.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

struct StubArray {
    utils::FixedArray<std::shared_ptr<grpc::Channel>> channels;
    mutable utils::FixedArray<ugrpc::impl::StubAny> stubs;
};

struct StubState {
    ClientQos client_qos;

    StubArray stubs;
    // method_id -> stub_pool
    utils::FixedArray<StubArray> dedicated_stubs;
};

const StubArray& GetMethodStubs(const StubState& stub_state, std::size_t method_id);

const StubArray& GetGenericMethodStubs(const StubState& stub_state);

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
