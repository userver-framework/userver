#include <userver/ugrpc/client/impl/stub_state.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

StubAny& NextStub(const StubState& stub_state, std::size_t method_id) {
    const auto& dedicated_stubs = stub_state.dedicated_stubs[method_id];
    const auto& stubs = dedicated_stubs.Size() ? dedicated_stubs : stub_state.stubs;
    return stubs.NextStub();
}

StubAny& NextStub(const StubState& stub_state) { return stub_state.stubs.NextStub(); }

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
