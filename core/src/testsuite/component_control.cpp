#include <userver/testsuite/component_control.hpp>

#include <userver/tracing/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

const std::string kInvalidatorSpanTag = "cache_invalidate";

}  // namespace

namespace testsuite {

void ComponentControl::RegisterComponentInvalidator(
    components::LoggableComponentBase& owner, Callback callback) {
  std::lock_guard lock(mutex_);
  invalidators_.emplace_back(owner, std::move(callback));
}

void ComponentControl::UnregisterComponentInvalidator(
    components::LoggableComponentBase& owner) {
  std::lock_guard lock(mutex_);
  invalidators_.remove_if([owner_ptr = &owner](const Invalidator& cand) {
    return cand.owner == owner_ptr;
  });
}

void ComponentControl::InvalidateComponents() {
  std::lock_guard lock(mutex_);

  for (const auto& invalidator : invalidators_) {
    tracing::Span span(kInvalidatorSpanTag);
    invalidator.callback();
  }
}

ComponentControl::Invalidator::Invalidator(
    components::LoggableComponentBase& owner_, Callback callback_)
    : owner(&owner_), callback(std::move(callback_)) {}

ComponentInvalidatorHolder::~ComponentInvalidatorHolder() {
  component_control_.UnregisterComponentInvalidator(component_);
}

}  // namespace testsuite

USERVER_NAMESPACE_END
