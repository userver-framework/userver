#pragma once

#include <userver/utils/impl/internal_tag.hpp>

USERVER_NAMESPACE_BEGIN

/// @cond For internal use only
class DeadlinePropagationMeta {
public:
    explicit DeadlinePropagationMeta(utils::impl::InternalTag) noexcept {}

    /// @cond For internal use only
    bool IsDeadlinePropagationCapped(utils::impl::InternalTag) const noexcept { return deadline_propagation_capped_; }
    /// @cond For internal use only
    void SetDeadlinePropagationCapped(utils::impl::InternalTag) noexcept { deadline_propagation_capped_ = true; }

private:
    /// Set by AdjustDeadline() when the command's timeout was capped to the
    /// inherited request deadline. Read by Request::Get() to signal deadline
    /// expiry to the middleware (MarkTaskInheritedDeadlineExpired).
    bool deadline_propagation_capped_{false};
};

USERVER_NAMESPACE_END
