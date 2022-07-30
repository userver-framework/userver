#include "handler_state.hpp"

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl {

void HandlerState::SetBroken() {
  broken_.store(true, std::memory_order_relaxed);
}

void HandlerState::SetBlocked() {
  blocked_.store(true, std::memory_order_relaxed);
}

void HandlerState::SetUnblocked() {
  blocked_.store(false, std::memory_order_relaxed);
}

bool HandlerState::IsBroken() const {
  return broken_.load(std::memory_order_relaxed);
}

bool HandlerState::IsBlocked() const {
  return blocked_.load(std::memory_order_relaxed);
}

}  // namespace urabbitmq::impl

USERVER_NAMESPACE_END
