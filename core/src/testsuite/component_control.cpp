#include <userver/testsuite/component_control.hpp>

#include <iterator>

#include <userver/tracing/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

const std::string kInvalidatorSpanTag = "cache_invalidate";

}  // namespace

namespace testsuite {

ComponentControl::InvalidatorHandle
ComponentControl::RegisterComponentInvalidator(Callback&& callback) {
  std::lock_guard lock(mutex_);
  invalidators_.emplace_back(std::move(callback));
  return std::prev(invalidators_.end());
}

void ComponentControl::UnregisterComponentInvalidator(
    InvalidatorHandle handle) noexcept {
  std::lock_guard lock(mutex_);
  invalidators_.erase(handle);
}

void ComponentControl::InvalidateComponents() {
  std::lock_guard lock(mutex_);

  for (const auto& invalidator : invalidators_) {
    tracing::Span span(kInvalidatorSpanTag);
    invalidator();
  }
}

ComponentInvalidatorHolder::~ComponentInvalidatorHolder() {
  component_control_.UnregisterComponentInvalidator(invalidator_handle_);
}

ComponentInvalidatorHolder::ComponentInvalidatorHolder(
    ComponentControl& component_control, std::function<void()>&& callback)
    : component_control_(component_control),
      invalidator_handle_(component_control_.RegisterComponentInvalidator(
          std::move(callback))) {}

}  // namespace testsuite

USERVER_NAMESPACE_END
