#pragma once

/// @file userver/concurrent/lazy_value.hpp
/// @brief @copybrief concurrent::LazyValue

#include <atomic>
#include <exception>
#include <utility>

#include <userver/engine/multi_consumer_event.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/move_only_function.hpp>
#include <userver/utils/result_store.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent {

/// @brief Lazy value computation with multiple consumers.
template <typename T>
class LazyValue final {
public:
    explicit LazyValue(utils::move_only_function<T()> f)
        : f_(std::move(f))
    {
        UASSERT(f_);
    }

    LazyValue(const LazyValue&) = delete;
    LazyValue(LazyValue&&) = delete;
    LazyValue& operator=(const LazyValue&) = delete;
    LazyValue& operator=(LazyValue&&) = delete;

    /// @brief Get an already calculated result or calculate it. It is guaranteed that `f` is called exactly once.
    /// Can be called concurrently from multiple coroutines.
    ///
    /// @note If `f` throws, it is not re-evaluated on subsequent calls (unlike with `std::once_flag`).
    ///
    /// @throws Anything `f` throws.
    const T& operator()();

private:
    utils::move_only_function<T()> f_;
    std::atomic<bool> started_{false};
    utils::ResultStore<T> result_;

    engine::MultiConsumerEvent finished_event_;
};

template <typename T>
const T& LazyValue<T>::operator()() {
    if (finished_event_.IsReady()) {
        return result_.Get();
    }

    const bool old = started_.exchange(true, std::memory_order_relaxed);
    if (!old) {
        try {
            result_.SetValue(f_());
        } catch (...) {
            result_.SetException(std::current_exception());
            finished_event_.Send();
            throw;
        }
        finished_event_.Send();
    } else {
        finished_event_.Wait();
    }

    return result_.Get();
}

}  // namespace concurrent

USERVER_NAMESPACE_END
