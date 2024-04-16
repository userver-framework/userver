#include <server/request/internal_request_context.hpp>

#include <stdexcept>

#include <utils/impl/assert_extra.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::request::impl {

void DeadlinePropagationContext::SetCancelledByDeadline() {
  is_cancelled_by_deadline_ = true;
}

bool DeadlinePropagationContext::IsCancelledByDeadline() const {
  return is_cancelled_by_deadline_;
}

void DeadlinePropagationContext::SetForcedLogLevel(logging::Level level) {
  forced_log_level_.emplace(level);
}

const std::optional<logging::Level>&
DeadlinePropagationContext::GetForcedLogLevel() const {
  return forced_log_level_;
}

InternalRequestContext::InternalRequestContext() = default;

InternalRequestContext::InternalRequestContext(
    InternalRequestContext&&) noexcept = default;

void InternalRequestContext::SetConfigSnapshot(
    dynamic_config::Snapshot&& config_snapshot) {
  config_snapshot_.emplace(std::move(config_snapshot));
}

const dynamic_config::Snapshot& InternalRequestContext::GetConfigSnapshot()
    const {
  if (!config_snapshot_.has_value()) {
    throw std::logic_error{"Config snapshot is already reset."};
  }

  return *config_snapshot_;
}

void InternalRequestContext::ResetConfigSnapshot() { config_snapshot_.reset(); }

DeadlinePropagationContext& InternalRequestContext::GetDPContext() {
  return dp_context_;
}

}  // namespace server::request::impl

USERVER_NAMESPACE_END
