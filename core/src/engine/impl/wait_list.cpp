#include "wait_list.hpp"

#include <boost/intrusive/list.hpp>

#include <engine/task/task_context.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

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

// not implicitly noexcept on focal
// NOLINTNEXTLINE(hicpp-use-equals-default,modernize-use-equals-default)
WaitList::WaitList() noexcept {}

WaitList::~WaitList() {
  UASSERT_MSG(waiting_contexts_->empty(), "Someone is waiting on the WaitList");
}

bool WaitList::IsEmpty(Lock& lock) const noexcept {
  UASSERT(lock);
  return waiting_contexts_->empty();
}

void WaitList::Append(
    Lock& lock, boost::intrusive_ptr<impl::TaskContext> context) noexcept {
  UASSERT(lock);
  UASSERT(context);
  UASSERT_MSG(!context->wait_list_hook.is_linked(), "context already in list");

  waiting_contexts_->push_back(*context.detach());  // referencing, not copying!
}

void WaitList::WakeupOne(Lock& lock) {
  UASSERT(lock);
  if (!waiting_contexts_->empty()) {
    boost::intrusive_ptr<impl::TaskContext> context(&waiting_contexts_->front(),
                                                    kAdopt);
    context->wait_list_hook.unlink();

    context->Wakeup(impl::TaskContext::WakeupSource::kWaitList,
                    impl::TaskContext::NoEpoch{});
  }
}

void WaitList::WakeupAll(Lock& lock) {
  UASSERT(lock);
  while (!waiting_contexts_->empty()) {
    boost::intrusive_ptr<impl::TaskContext> context(&waiting_contexts_->front(),
                                                    kAdopt);
    context->wait_list_hook.unlink();

    context->Wakeup(impl::TaskContext::WakeupSource::kWaitList,
                    impl::TaskContext::NoEpoch{});
  }
}

void WaitList::Remove(Lock& lock, impl::TaskContext& context) noexcept {
  UASSERT(lock);
  if (!context.wait_list_hook.is_linked()) return;

  boost::intrusive_ptr<impl::TaskContext> ctx(&context, kAdopt);
  UASSERT_MSG(IsInIntrusiveContainer(*waiting_contexts_, context),
              "context belongs to other list");

  context.wait_list_hook.unlink();
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
