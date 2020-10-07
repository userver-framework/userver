#include "wait_list.hpp"

#include <algorithm>
#include <boost/intrusive/list.hpp>

#include <engine/task/task_context.hpp>

#include <utils/assert.hpp>

namespace engine::impl {

namespace {

constexpr bool kAdopt = false;

template <class Container, class Value>
bool IsInIntrusiveContainer(const Container& container, const Value& val) {
  const auto val_it = Container::s_iterator_to(val);
  for (auto it = container.cbegin(); it != container.cend(); ++it) {
    if (it == val_it) {
      return true;
    }
  }

  return false;
}

using MemberHookConfig =
    boost::intrusive::member_hook<impl::TaskContext,
                                  impl::TaskContext::WaitListHook,
                                  &impl::TaskContext::wait_list_hook>;

}  // namespace

struct WaitList::List
    : public boost::intrusive::make_list<
          impl::TaskContext, boost::intrusive::constant_time_size<false>,
          MemberHookConfig>::type {};

WaitList::WaitList() : waiting_contexts_() {}

WaitList::~WaitList() {
  UASSERT_MSG(waiting_contexts_->empty(), "Someone is waiting on the WaitList");
}

bool WaitList::IsEmpty([[maybe_unused]] WaitListBase::Lock& lock) const {
  UASSERT(lock);
  return waiting_contexts_->empty();
}

void WaitList::Append([[maybe_unused]] WaitListBase::Lock& lock,
                      boost::intrusive_ptr<impl::TaskContext> context) {
  UASSERT(lock);
  UASSERT_MSG(!context->wait_list_hook.is_linked(), "context already in list");
  waiting_contexts_->push_back(*context.detach());  // referencing, not copying!
}

void WaitList::WakeupOne([[maybe_unused]] WaitListBase::Lock& lock) {
  UASSERT(lock);
  if (!waiting_contexts_->empty()) {
    boost::intrusive_ptr<impl::TaskContext> context(&waiting_contexts_->front(),
                                                    kAdopt);
    context->wait_list_hook.unlink();

    context->Wakeup(impl::TaskContext::WakeupSource::kWaitList,
                    impl::TaskContext::NoEpoch{});
  }
}

void WaitList::WakeupAll([[maybe_unused]] WaitListBase::Lock& lock) {
  UASSERT(lock);
  while (!waiting_contexts_->empty()) {
    boost::intrusive_ptr<impl::TaskContext> context(&waiting_contexts_->front(),
                                                    kAdopt);
    context->wait_list_hook.unlink();

    context->Wakeup(impl::TaskContext::WakeupSource::kWaitList,
                    impl::TaskContext::NoEpoch{});
  }
}

void WaitList::Remove(const boost::intrusive_ptr<impl::TaskContext>& context) {
  Lock lock(*this);
  if (!context->wait_list_hook.is_linked()) return;
  boost::intrusive_ptr<impl::TaskContext> ctx(context.get(), kAdopt);

  UASSERT_MSG(IsInIntrusiveContainer(*waiting_contexts_, *context),
              "context belongs to other list");

  context->wait_list_hook.unlink();
}

}  // namespace engine::impl
