#pragma once

/// @file userver/engine/wait_any.hpp
/// @brief Provides engine::WaitAny, engine::WaitAnyFor, engine::WaitAnyUntil, engine::WaitAnyContext and
/// engine::MakeWaitAny

#include <chrono>
#include <cstdint>
#include <optional>
#include <vector>

#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <userver/engine/awaitable.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/utils/expected.hpp>
#include <userver/utils/fast_pimpl.hpp>
#include <userver/utils/meta.hpp>
#include <userver/utils/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

/// @ingroup userver_concurrency
///
/// @deprecated `WaitAny` is deprecated. Please, prefer @ref MakeWaitAny + @ref WaitAnyContext::Wait.
/// @brief Waits for the completion of any of the specified tasks or the
/// cancellation of the caller.
///
/// Could be used to get the ready HTTP requests ASAP:
/// @snippet src/clients/http/client_wait_test.cpp HTTP Client - waitany
///
/// Works with different types of tasks and futures:
/// @snippet src/engine/wait_any_test.cpp sample waitany
///
/// @param tasks either a single container, or a pack of future-like elements.
/// @returns the index of the completed task, or `std::nullopt` if there are no
/// completed tasks (possible if current task was cancelled).
template <typename... Tasks>
std::optional<std::size_t> WaitAny(Tasks&... tasks);

/// @ingroup userver_concurrency
///
/// @deprecated `WaitAnyFor` is deprecated. Please, prefer @ref MakeWaitAny + @ref WaitAnyContext::WaitFor.
/// @overload std::optional<std::size_t> WaitAny(Tasks&... tasks)
template <typename... Tasks, typename Rep, typename Period>
std::optional<std::size_t> WaitAnyFor(const std::chrono::duration<Rep, Period>& duration, Tasks&... tasks);

/// @ingroup userver_concurrency
///
/// @deprecated `WaitAnyUntil` is deprecated. Please, prefer @ref MakeWaitAny + @ref WaitAnyContext::WaitUntil.
/// @overload std::optional<std::size_t> WaitAny(Tasks&... tasks)
template <typename... Tasks, typename Clock, typename Duration>
std::optional<std::size_t> WaitAnyUntil(const std::chrono::time_point<Clock, Duration>& until, Tasks&... tasks);

/// @ingroup userver_concurrency
///
/// @deprecated `WaitAnyUntil` is deprecated. Please, prefer @ref MakeWaitAny + @ref WaitAnyContext::WaitUntil.
/// @overload std::optional<std::size_t> WaitAny(Tasks&... tasks)
template <typename... Tasks>
std::optional<std::size_t> WaitAnyUntil(Deadline, Tasks&... tasks);

template <typename... Tasks>
std::optional<std::size_t> WaitAny(Tasks&... tasks) {
    return engine::WaitAnyUntil(Deadline{}, tasks...);
}

template <typename... Tasks, typename Rep, typename Period>
std::optional<std::size_t> WaitAnyFor(const std::chrono::duration<Rep, Period>& duration, Tasks&... tasks) {
    return engine::WaitAnyUntil(Deadline::FromDuration(duration), tasks...);
}

template <typename... Tasks, typename Clock, typename Duration>
std::optional<std::size_t> WaitAnyUntil(const std::chrono::time_point<Clock, Duration>& until, Tasks&... tasks) {
    return engine::WaitAnyUntil(Deadline::FromTimePoint(until), tasks...);
}

namespace impl {

std::optional<std::size_t> DoWaitAny(utils::span<AwaitableToken> target_tokens, Deadline deadline);

template <typename Container>
std::optional<std::size_t> WaitAnyFromContainer(Deadline deadline, Container& tasks) {
    const auto size = std::size(tasks);
    std::vector<AwaitableToken> targets;
    targets.reserve(size);

    for (auto& task : tasks) {
        static_assert(engine::Awaitable<decltype(task)>, "Tasks must be awaitable");
        targets.push_back(task.GetAwaitableToken());
    }

    return DoWaitAny(targets, deadline);
}

template <typename... Tasks>
std::optional<std::size_t> WaitAnyFromTasks(Deadline deadline, Tasks&... tasks) {
    static_assert((true && ... && engine::Awaitable<Tasks>), "Tasks must be awaitable");
    AwaitableToken wa_elements[]{tasks.GetAwaitableToken()...};
    return DoWaitAny(wa_elements, deadline);
}

inline std::optional<std::size_t> WaitAnyFromTasks(Deadline) { return {}; }

}  // namespace impl

template <typename... Tasks>
std::optional<std::size_t> WaitAnyUntil(Deadline deadline, Tasks&... tasks) {
    if constexpr (meta::impl::IsSingleRange<Tasks...>) {
        return impl::WaitAnyFromContainer(deadline, tasks...);
    } else {
        return impl::WaitAnyFromTasks(deadline, tasks...);
    }
}

/// @ingroup userver_concurrency
///
/// @brief The reason why @ref WaitAnyContext::Wait and friends did not return the index of a completed awaitable.
enum class WaitAnyError : std::uint8_t {
    kEmpty,      ///< there were no awaitables to wait for
    kCancelled,  ///< the wait operation was interrupted by task cancellation
    kTimeout,    ///< the wait operation timed out (see also @ref FutureStatus)
};

/// @ingroup userver_concurrency
///
/// @brief Stores a set of awaitables and allows waiting for completion of any of the stored awaitables.
///
/// Works with different types of awaitables:
/// @snippet src/engine/wait_any_test.cpp sample MakeWaitAny
///
/// No methods (except .dtor) should be called on a moved-out instance.
class WaitAnyContext final {
public:
    WaitAnyContext();
    ~WaitAnyContext() noexcept;
    WaitAnyContext(WaitAnyContext&&) noexcept;
    WaitAnyContext& operator=(WaitAnyContext&& other) noexcept;
    WaitAnyContext(const WaitAnyContext&) = delete;
    WaitAnyContext& operator=(const WaitAnyContext&) = delete;

    /// @brief Appends a single awaitable to the context.
    ///
    /// The appended awaitable will be assigned the next id from the auto-incrementing counter;
    /// see also @ref GetNextId.
    void Append(engine::Awaitable auto& awaitable);

    /// @brief Appends the given awaitable to the context with an explicit id.
    ///
    /// The id is an arbitrary number that will be returned by @ref Wait.
    /// It does not have to be sequential or starting from 0. Duplicate ids are allowed.
    ///
    /// @ref GetNextId will be unaffected by this call. When mixing the id-less @ref Append
    /// and this overload, the id-less `Append` maintains a sequence only within its own calls.
    ///
    /// Works well together with @ref utils::SlotMap to process and erase tasks in completion order:
    /// @snippet src/engine/wait_any_test.cpp sample WaitAnyContext SlotMap
    ///
    /// @param id the id that will be returned by @ref Wait.
    /// @param awaitable the awaitable to append.
    void Append(std::uint64_t id, engine::Awaitable auto& awaitable);

    /// @brief Waits either for the completion of any of the awaitables stored in the context
    /// or for the cancellation of the caller.
    ///
    /// @returns the index of the completed awaitable, or a @ref WaitAnyError if there are no
    /// completed awaitables (possible if current task was cancelled or the context is empty).
    utils::expected<std::uint64_t, WaitAnyError> Wait();

    /// @brief Waits for the completion of any of the awaitables stored in the context
    /// or cancellation of the caller or deadline expiration.
    ///
    /// @returns the index of the completed awaitable, or a @ref WaitAnyError if there are no
    /// completed awaitables (possible if current task was cancelled, the deadline was reached,
    /// or the context is empty).
    utils::expected<std::uint64_t, WaitAnyError> WaitUntil(Deadline deadline);

    /// @brief Waits for the completion of any of the awaitables stored in the context
    /// or cancellation of the caller or expiration of the given duration.
    ///
    /// @returns the index of the completed awaitable, or a @ref WaitAnyError if there are no
    /// completed awaitables (possible if current task was cancelled, the given duration has passed,
    /// or the context is empty).
    template <typename Rep, typename Period>
    utils::expected<std::uint64_t, WaitAnyError> WaitFor(const std::chrono::duration<Rep, Period>& duration) {
        return WaitUntil(Deadline::FromDuration(duration));
    }

    /// @overload utils::expected<std::uint64_t, WaitAnyError> Wait()
    template <typename Clock, typename Duration>
    utils::expected<std::uint64_t, WaitAnyError> WaitUntil(const std::chrono::time_point<Clock, Duration>& until) {
        return WaitUntil(Deadline::FromTimePoint(until));
    }

    /// @brief Returns the number of awaitables actually stored in the context.
    ///
    /// It consists of actively awaited and pending subscription awaitables.
    /// Already notified awaitables are dropped out.
    std::size_t GetSize() const noexcept;

    /// @brief Returns the next id that will be assigned by the next @ref Append call.
    ///
    /// It could be used to calculate ids of awaitables appended via @ref Append call.
    std::uint64_t GetNextId() const noexcept;

private:
    class Impl;

    void AppendToken(engine::AwaitableToken awaitable);

    void AppendToken(std::uint64_t id, engine::AwaitableToken awaitable);

    boost::intrusive_ptr<Impl> impl_;
};

void WaitAnyContext::Append(engine::Awaitable auto& awaitable) { AppendToken(awaitable.GetAwaitableToken()); }

void WaitAnyContext::Append(std::uint64_t id, engine::Awaitable auto& awaitable) {
    AppendToken(id, awaitable.GetAwaitableToken());
}

/// @ingroup userver_concurrency
///
/// @brief Produces a WaitAnyContext for the given awaitables and sequences of awaitables.
///
/// Each passed awaitable could be either a single awaitable or a container of awaitables.
/// In the latter case all awaitables from the container are appended to the context.
/// The stored awaitables will have ids [0, GetNextId() - 1].
template <typename... Awaitables>
WaitAnyContext MakeWaitAny(Awaitables&... awaitables) {
    auto context = WaitAnyContext();
    [[maybe_unused]] const auto append_one = [&context](auto& arg) {
        if constexpr (meta::IsRange<decltype(arg)>) {
            for (auto& awaitable : arg) {
                context.Append(awaitable);
            }
        } else {
            context.Append(arg);
        }
    };
    (append_one(awaitables), ...);
    return context;
}

}  // namespace engine

USERVER_NAMESPACE_END
