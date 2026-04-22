#pragma once

#include <userver/engine/impl/context_accessor.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

// Can be used to make a mocked future pre-filled with a value.
class ReadyContextAccessor final : public ContextAccessor {
public:
    ReadyContextAccessor() = default;

    bool IsReady() const noexcept override { return true; }

    void TryAppendAwaiter(boost::intrusive_ptr<Awaiter>&, std::uintptr_t) override {}

    void RemoveAwaiter(engine::impl::Awaiter&, std::uintptr_t) noexcept override {}
};

}  // namespace engine::impl

USERVER_NAMESPACE_END
