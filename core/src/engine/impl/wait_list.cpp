#include "wait_list.hpp"

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

}  // namespace

// not implicitly noexcept on focal
// NOLINTNEXTLINE(hicpp-use-equals-default,modernize-use-equals-default)
WaitList::WaitList() noexcept {}

WaitList::~WaitList() { UASSERT_MSG(awaiters_.empty(), "Someone is waiting on the WaitList"); }

bool WaitList::IsEmpty(Lock& lock) const noexcept {
    UASSERT(lock);
    return awaiters_.empty();
}

void WaitList::Append(Lock& lock, boost::intrusive_ptr<impl::Awaiter> awaiter, std::uintptr_t context) noexcept {
    UASSERT(lock);
    UASSERT(awaiter);
    UASSERT_MSG(!awaiter->wait_list_data_.is_linked(), "context already in list");

    awaiter->wait_list_data_.context = context;
    awaiters_.push_back(*awaiter.detach());  // referencing, not copying!
}

void WaitList::NotifyOne(Lock& lock) {
    UASSERT(lock);
    if (awaiters_.empty()) {
        return;
    }
    boost::intrusive_ptr<impl::Awaiter> awaiter(&awaiters_.front(), kAdopt);
    awaiter->wait_list_data_.unlink();
    const auto context = awaiter->wait_list_data_.context;
    impl::Notify(std::move(awaiter), context);
}

void WaitList::NotifyAll(Lock& lock) {
    UASSERT(lock);
    while (!awaiters_.empty()) {
        boost::intrusive_ptr<impl::Awaiter> awaiter(&awaiters_.front(), kAdopt);
        awaiter->wait_list_data_.unlink();
        const auto context = awaiter->wait_list_data_.context;
        impl::Notify(std::move(awaiter), context);
    }
}

void WaitList::Remove(Lock& lock, impl::Awaiter& awaiter, std::uintptr_t) noexcept {
    UASSERT(lock);
    if (!awaiter.wait_list_data_.is_linked()) {
        return;
    }

    const boost::intrusive_ptr<impl::Awaiter> holder(&awaiter, kAdopt);
    UASSERT_MSG(IsInIntrusiveContainer(awaiters_, awaiter), "awaiter belongs to other list");

    awaiter.wait_list_data_.unlink();
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
