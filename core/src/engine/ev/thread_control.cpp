#include <engine/ev/thread_control.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::ev {

void ThreadControl::RunInEvLoopAsync(
    OnRefcountedPayload* func,
    boost::intrusive_ptr<IntrusiveRefcountedBase>&& data) {
  thread_.RunInEvLoopAsync(func, std::move(data));
}

void ThreadControl::RunInEvLoopDeferred(
    OnRefcountedPayload* func,
    boost::intrusive_ptr<IntrusiveRefcountedBase>&& data, Deadline deadline) {
  thread_.RunInEvLoopDeferred(func, std::move(data), deadline);
}

}  // namespace engine::ev

USERVER_NAMESPACE_END
