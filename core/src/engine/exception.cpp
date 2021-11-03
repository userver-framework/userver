#include <userver/engine/exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

WaitInterruptedException::WaitInterruptedException(
    TaskCancellationReason reason)
    : std::runtime_error(
          "Wait interrupted because of task cancellation, reason=" +
          ToString(reason)),
      reason_(reason) {}

TaskCancellationReason WaitInterruptedException::Reason() const noexcept {
  return reason_;
}

TaskCancelledException::TaskCancelledException(TaskCancellationReason reason)
    : std::runtime_error("Task cancelled, reason=" + ToString(reason)),
      reason_(reason) {}

TaskCancellationReason TaskCancelledException::Reason() const noexcept {
  return reason_;
}

}  // namespace engine

USERVER_NAMESPACE_END
