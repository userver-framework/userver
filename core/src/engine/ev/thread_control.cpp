#include <engine/ev/thread_control.hpp>

namespace engine::ev {

void ThreadControl::RunInEvLoopAsync(
    OnRefcountedPayload* func,
    boost::intrusive_ptr<IntrusiveRefcountedBase>&& data) {
  thread_.RunInEvLoopAsync(func, std::move(data));
}

}  // namespace engine::ev
