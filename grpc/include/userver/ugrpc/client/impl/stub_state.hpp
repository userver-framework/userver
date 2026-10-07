#pragma once

#include <string>

#include <grpcpp/channel.h>
#include <grpcpp/generic/generic_stub.h>

#include <userver/utils/fixed_array.hpp>

#include <userver/ugrpc/client/auth_type.hpp>
#include <userver/ugrpc/client/client_qos.hpp>
#include <userver/ugrpc/impl/stub_any.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

struct StubArray {
    utils::FixedArray<std::shared_ptr<grpc::Channel>> channels;
    mutable utils::FixedArray<ugrpc::impl::StubAny> stubs;
    mutable utils::FixedArray<grpc::GenericStub> generic_stubs;
};

struct StubState {
    ClientQos client_qos;
    std::string endpoint;
    AuthType auth_type{};

    StubArray stubs;
    // method_id -> stub_pool
    utils::FixedArray<StubArray> dedicated_stubs;
};

const StubArray& GetMethodStubs(const StubState& stub_state, std::size_t method_id);

const StubArray& GetGenericMethodStubs(const StubState& stub_state);

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
