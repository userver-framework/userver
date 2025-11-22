#pragma once

#include <userver/rcu/rcu.hpp>

#include <userver/ugrpc/client/impl/stub_any.hpp>
#include <userver/ugrpc/client/impl/stub_state.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

struct StubState;

class StubHandle final {
public:
    StubHandle(rcu::ReadablePtr<StubState>&& state, StubAny& stub);

    StubHandle(StubHandle&&) noexcept = default;
    StubHandle& operator=(StubHandle&&) = delete;

    StubHandle(const StubHandle&) = delete;
    StubHandle& operator=(const StubHandle&) = delete;

    template <typename Stub>
    Stub& Get() {
        return StubCast<Stub>(stub_);
    }

    const ClientQos& GetClientQos() const;

private:
    const rcu::ReadablePtr<StubState> state_;
    StubAny& stub_;
};

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
