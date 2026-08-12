#include <userver/ugrpc/client/impl/stub_state.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

const StubArray& GetMethodStubs(const StubState& stub_state, std::size_t method_id) {
    const auto& dedicated_stubs = stub_state.dedicated_stubs[method_id];
    return !dedicated_stubs.stubs.empty() ? dedicated_stubs : stub_state.stubs;
}

const StubArray& GetGenericMethodStubs(const StubState& stub_state) { return stub_state.stubs; }

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
