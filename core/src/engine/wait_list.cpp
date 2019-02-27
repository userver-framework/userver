#include "wait_list.hpp"

#include <algorithm>

#include <engine/task/task_context.hpp>

#include <utils/assert.hpp>

namespace engine {
namespace impl {

void WaitList::Append([[maybe_unused]] WaitListBase::Lock& lock,
                      boost::intrusive_ptr<impl::TaskContext> context) {
  UASSERT(lock);
  UASSERT(std::find(waiting_contexts_.begin(), waiting_contexts_.end(),
                    context) == waiting_contexts_.end());
  waiting_contexts_.push_back(std::move(context));
}

void WaitList::WakeupOne([[maybe_unused]] WaitListBase::Lock& lock) {
  UASSERT(lock);
  SkipRemoved(lock);
  if (!waiting_contexts_.empty()) {
    UASSERT(waiting_contexts_.front());
    waiting_contexts_.front()->Wakeup(
        impl::TaskContext::WakeupSource::kWaitList);
    waiting_contexts_.pop_front();
  }
}

void WaitList::WakeupAll([[maybe_unused]] WaitListBase::Lock& lock) {
  UASSERT(lock);
  for (auto& context : waiting_contexts_) {
    if (context) {
      context->Wakeup(impl::TaskContext::WakeupSource::kWaitList);
    }
  }
  waiting_contexts_.clear();
}

void WaitList::Remove(const boost::intrusive_ptr<impl::TaskContext>& context) {
  Lock lock(*this);

  auto it =
      std::find(waiting_contexts_.begin(), waiting_contexts_.end(), context);
  if (it == waiting_contexts_.end()) return;

  it->reset();
  UASSERT(std::find(std::next(it), waiting_contexts_.end(), context) ==
          waiting_contexts_.end());
}

void WaitList::SkipRemoved([[maybe_unused]] WaitListBase::Lock& lock) {
  UASSERT(lock);
  while (!waiting_contexts_.empty() && !waiting_contexts_.front()) {
    waiting_contexts_.pop_front();
  }
}

}  // namespace impl
}  // namespace engine
