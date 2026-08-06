#pragma once

#include <atomic>
#include <cstddef>
#include <memory>

#include <boost/smart_ptr/intrusive_ptr.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

// Same as boost::intrusive_ref_counter, but starts from 1, avoiding an extra atomic increment.
// Requires the use of MakeIntrusiveOne to create the object.
template <typename Derived, typename Deleter = std::default_delete<Derived>>
class IntrusiveRefCounterOne {
public:
    IntrusiveRefCounterOne(const IntrusiveRefCounterOne&) = delete;
    IntrusiveRefCounterOne(IntrusiveRefCounterOne&&) = delete;
    IntrusiveRefCounterOne& operator=(const IntrusiveRefCounterOne&) = delete;
    IntrusiveRefCounterOne& operator=(IntrusiveRefCounterOne&&) = delete;

    // NOLINTNEXTLINE(readability-identifier-naming)
    friend void intrusive_ptr_add_ref(Derived* self) noexcept {
        auto& base = static_cast<IntrusiveRefCounterOne&>(*self);
        base.intrusive_ref_count_.fetch_add(1, std::memory_order_seq_cst);
    }

    // NOLINTNEXTLINE(readability-identifier-naming)
    friend void intrusive_ptr_release(Derived* self) noexcept {
        auto& base = static_cast<IntrusiveRefCounterOne&>(*self);
        if (base.intrusive_ref_count_.fetch_sub(1, std::memory_order_seq_cst) == 1) {
            Deleter{}(self);
        }
    }

    std::size_t UseCount() const noexcept { return intrusive_ref_count_.load(std::memory_order_relaxed); }

protected:
    IntrusiveRefCounterOne() = default;
    ~IntrusiveRefCounterOne() = default;

private:
    std::atomic<std::size_t> intrusive_ref_count_{1};
};

template <typename T, typename... Args>
boost::intrusive_ptr<T> MakeIntrusiveOne(Args&&... args) {
    // The initial ref count is already 1, so no extra intrusive_ptr_add_ref call is needed here.
    return boost::intrusive_ptr<T>(new T(std::forward<Args>(args)...), /*add_ref=*/false);
}

}  // namespace utils::impl

USERVER_NAMESPACE_END
