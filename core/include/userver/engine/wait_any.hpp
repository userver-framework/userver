#pragma once

/// @file userver/engine/wait_any.hpp
/// @brief Provides engine::WaitAny, engine::WaitAnyFor, engine::WaitAnyUntil, engine::WaitAnyContext and
/// engine::MakeWaitAny

#include <chrono>
#include <cstdint>
#include <optional>
#include <vector>

#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <userver/engine/deadline.hpp>
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

class ContextAccessor;

std::optional<std::size_t> DoWaitAny(utils::span<ContextAccessor*> targets, Deadline deadline);

template <typename Container>
std::optional<std::size_t> WaitAnyFromContainer(Deadline deadline, Container& tasks) {
    const auto size = std::size(tasks);
    std::vector<ContextAccessor*> targets;
    targets.reserve(size);

    for (auto& task : tasks) {
        targets.push_back(task.TryGetContextAccessor());
    }

    return DoWaitAny(targets, deadline);
}

template <typename... Tasks>
std::optional<std::size_t> WaitAnyFromTasks(Deadline deadline, Tasks&... tasks) {
    ContextAccessor* wa_elements[]{tasks.TryGetContextAccessor()...};
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

    /// @brief Append the given awaitables and sequences of awaitables to the context.
    ///
    /// Each passed awaitable could be either a single awaitable or a container of awaitables.
    /// In the latter case all awaitables from the container are appended to the context.
    /// The appended awaitables will have indexes [GetNextIndex() before the call, GetNextIndex() after the call - 1].
    template <typename... Awaitables>
    void Append(Awaitables&... awaitables);

    /// @brief Waits either for the completion of any of the awaitables stored in the context
    /// or for the cancellation of the caller.
    ///
    /// @returns the index of the completed awaitable, or `std::nullopt` if there are no
    /// completed awaitables (possible if current task was cancelled).
    std::optional<std::uint64_t> Wait();

    /// @brief Waits for the completion of any of the awaitables stored in the context
    /// or cancellation of the caller or deadline expiration.
    ///
    /// @returns the index of the completed awaitable, or `std::nullopt` if there are no
    /// completed awaitables (possible if current task was cancelled or the deadline was reached).
    std::optional<std::uint64_t> WaitUntil(Deadline deadline);

    /// @brief Waits for the completion of any of the awaitables stored in the context
    /// or cancellation of the caller or expiration of the given duration.
    ///
    /// @returns the index of the completed awaitable, or `std::nullopt` if there are no
    /// completed awaitables (possible if current task was cancelled or the given duration has passed).
    template <typename Rep, typename Period>
    std::optional<std::uint64_t> WaitFor(const std::chrono::duration<Rep, Period>& duration) {
        return WaitUntil(Deadline::FromDuration(duration));
    }

    /// @overload std::optional<std::size_t> Wait()
    template <typename Clock, typename Duration>
    std::optional<std::uint64_t> WaitUntil(const std::chrono::time_point<Clock, Duration>& until) {
        return WaitUntil(Deadline::FromTimePoint(until));
    }

    /// @brief Returns the number of awaitables actually stored in the context.
    ///
    /// It consists of actively awaited and pending subscription awaitables.
    /// Already notified awaitables are dropped out.
    std::size_t GetSize() const noexcept;

    /// @brief Returns the next awaitable index.
    ///
    /// It could be used to calculate indexes of awaitables appended via Append call.
    std::uint64_t GetNextIndex() const noexcept;

private:
    class Impl;

    template <typename Container>
    void AppendFromContainer(Container& awaitables);

    template <typename Awaitable>
    void AppendSingle(Awaitable& awaitable);

    void AppendAccessor(impl::ContextAccessor* awaitable);

    boost::intrusive_ptr<Impl> impl_;
};

template <typename Container>
void WaitAnyContext::AppendFromContainer(Container& awaitables) {
    for (auto& awaitable : awaitables) {
        AppendAccessor(awaitable.TryGetContextAccessor());
    }
}

template <typename Awaitable>
void WaitAnyContext::AppendSingle(Awaitable& awaitable) {
    if constexpr (meta::impl::IsSingleRange<Awaitable>) {
        AppendFromContainer(awaitable);
    } else {
        AppendAccessor(awaitable.TryGetContextAccessor());
    }
}

template <typename... Awaitables>
void WaitAnyContext::Append(Awaitables&... awaitables) {
    (AppendSingle(awaitables), ...);
}

/// @ingroup userver_concurrency
///
/// @brief Produces a WaitAnyContext for the given awaitables and sequences of awaitables.
///
/// Each passed awaitable could be either a single awaitable or a container of awaitables.
/// In the latter case all awaitables from the container are appended to the context.
/// The stored awaitables will have indexes [0, GetNextIndex() - 1].
template <typename... Awaitables>
WaitAnyContext MakeWaitAny(Awaitables&... awaitables) {
    auto context = WaitAnyContext();
    if constexpr (sizeof...(Awaitables) > 0) {
        context.Append(awaitables...);
    }
    return context;
}

}  // namespace engine

USERVER_NAMESPACE_END
