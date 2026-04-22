#pragma once

#include <optional>

#include <userver/rcu/rcu.hpp>

#include <userver/ugrpc/impl/stub_any.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

struct StubState;
struct StubArray;

class MethodStubs final {
public:
    MethodStubs(rcu::ReadablePtr<StubState>&& stub_state, const StubArray& stubs);

    MethodStubs(MethodStubs&&) noexcept = default;
    MethodStubs& operator=(MethodStubs&&) = delete;

    MethodStubs(const MethodStubs&) = delete;
    MethodStubs& operator=(const MethodStubs&) = delete;

    ugrpc::impl::StubAny& GetStub() const;

private:
    const rcu::ReadablePtr<StubState> stub_state_;
    const StubArray& stubs_;
    mutable std::optional<std::size_t> last_;
};

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
