#include <engine/ev/thread_control.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::ev {

void ThreadControl::RunInEvLoopAsync(
    OnRefcountedPayload* func,
    boost::intrusive_ptr<IntrusiveRefcountedBase>&& data) {
  thread_.RunInEvLoopAsync(func, std::move(data));
}

}  // namespace engine::ev

USERVER_NAMESPACE_END
