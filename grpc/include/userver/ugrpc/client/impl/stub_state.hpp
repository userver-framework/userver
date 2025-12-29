#pragma once

#include <userver/utils/fixed_array.hpp>

#include <userver/ugrpc/client/client_qos.hpp>
#include <userver/ugrpc/client/impl/stub_pool.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

struct StubState {
    ClientQos client_qos;

    StubPool stubs;
    // method_id -> stub_pool
    utils::FixedArray<StubPool> dedicated_stubs;
};

StubAny& NextStub(const StubState& stub_state, std::size_t method_id);

StubAny& NextStub(const StubState& stub_state);

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
