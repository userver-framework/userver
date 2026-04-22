#pragma once

/// @file userver/concurrent/async_event_channel.hpp
/// @brief @copybrief concurrent::AsyncEventChannel

#include <functional>
#include <string>
#include <string_view>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

#include <userver/concurrent/async_event_source.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/engine/semaphore.hpp>
#include <userver/engine/shared_mutex.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent {

namespace impl {

void WaitForTask(std::string_view name, engine::TaskWithResult<void>& task);

[[noreturn]] void ReportAlreadySubscribed(std::string_view channel_name, std::string_view listener_name);

void ReportNotSubscribed(std::string_view channel_name) noexcept;

void ReportUnsubscribingAutomatically(std::string_view channel_name, std::string_view listener_name) noexcept;

void ReportErrorWhileUnsubscribing(
    std::string_view channel_name,
    std::string_view listener_name,
    std::string_view error
) noexcept;

std::string MakeAsyncChannelName(std::string_view base, std::string_view name);

inline constexpr bool kCheckSubscriptionUB = utils::impl::kEnableAssert;

// During the `AsyncEventSubscriberScope::Unsubscribe` call or destruction of
// `AsyncEventSubscriberScope`, all variables used by callback must be valid
// (must not be destroyed). A common cause of crashes in this place: there is no
// manual call to `Unsubscribe`. In this case check the declaration order of the
// struct fields.
template <typename Func>
void CheckDataUsedByCallbackHasNotBeenDestroyedBeforeUnsubscribing(
    std::function<void(const Func&)>& on_listener_removal,
    const Func& listener_func,
    std::string_view channel_name,
    std::string_view listener_name
) noexcept {
    if (!on_listener_removal) {
        return;
    }
    try {
        on_listener_removal(listener_func);
    } catch (const std::exception& e) {
        ReportErrorWhileUnsubscribing(channel_name, listener_name, e.what());
    }
}

}  // namespace impl

/// @ingroup userver_concurrency
///
/// AsyncEventChannel is an in-process pub-sub with strict FIFO serialization,
/// i.e. only after the event was processed a new event may appear for
/// processing, same listener is never called concurrently.
///
/// Example usage:
/// @snippet concurrent/async_event_channel_test.cpp  AsyncEventChannel sample
template <typename... Args>
class AsyncEventChannel : public AsyncEventSource<Args...> {
public:
    using Function = typename AsyncEventSource<Args...>::Function;
    using OnRemoveCallback = std::function<void(const Function&)>;

    /// @brief The primary constructor
    /// @param name used for diagnostic purposes and is also accessible with Name
    explicit AsyncEventChannel(std::string_view name)
        : name_(name),
          data_(ListenersData{{}, {}})
    {}

    /// @brief The constructor with `AsyncEventSubscriberScope` usage checking.
    ///
    /// The constructor with a callback that is called on listener removal. The
    /// callback takes a reference to `Function` as input. This is useful for
    /// checking the lifetime of data captured by the listener update function.
    ///
    /// @note Works only in debug mode.
    ///
    /// @warning Data captured by `on_listener_removal` function must be valid
    /// until the `AsyncEventChannel` object is completely destroyed.
    ///
    /// Example usage:
    /// @snippet concurrent/async_event_channel_test.cpp OnListenerRemoval sample
    ///
    /// @param name used for diagnostic purposes and is also accessible with Name
    /// @param on_listener_removal the callback used for check
    ///
    /// @see impl::CheckDataUsedByCallbackHasNotBeenDestroyedBeforeUnsubscribing
    AsyncEventChannel(std::string_view name, OnRemoveCallback on_listener_removal)
        : name_(name),
          data_(ListenersData{{}, std::move(on_listener_removal)})
    {}

    /// @brief For use in `UpdateAndListen` of specific event channels
    ///
    /// Atomically calls `updater`, which should invoke `func` with the previously
    /// sent event, and subscribes to new events as if using AddListener.
    ///
    /// @param id the subscriber class instance, see also a simpler `DoUpdateAndListen` overload below
    /// @param name the name of the subscriber
    /// @param func the callback that is called on each update
    /// @param updater the initial `() -> void` callback that should call `func` with the current value
    ///
    /// @see AsyncEventSource::AddListener
    template <typename UpdaterFunc>
    AsyncEventSubscriberScope DoUpdateAndListen(
        FunctionId id,
        std::string_view name,
        Function&& func,
        UpdaterFunc&& updater
    ) {
        const std::shared_lock lock(event_mutex_);
        std::forward<UpdaterFunc>(updater)();
        return DoAddListener(id, name, std::move(func));
    }

    /// @overload
    template <typename Class, typename UpdaterFunc>
    AsyncEventSubscriberScope DoUpdateAndListen(
        Class* obj,
        std::string_view name,
        void (Class::*func)(Args...),
        UpdaterFunc&& updater
    ) {
        return DoUpdateAndListen(
            FunctionId(obj),
            name,
            [obj, func](Args... args) { (obj->*func)(args...); },
            std::forward<UpdaterFunc>(updater)
        );
    }

    /// Send the next event and wait until all the listeners process it.
    ///
    /// Strict FIFO serialization is guaranteed, i.e. only after this event is
    /// processed a new event may be delivered for the subscribers, same
    /// listener/subscriber is never called concurrently.
    void SendEvent(Args... args) const {
        struct Task {
            std::shared_ptr<const Listener> listener;
            engine::TaskWithResult<void> task;
        };
        std::vector<Task> tasks;

        // Try to obtain unique lock for event_mutex_ to serialize
        // calls to SendEvent()
        event_mutex_.lock();

        // Now downgrade the lock to shared to allow new subscriptions
        event_mutex_.unlock_and_lock_shared();

        // And ensure the lock releases in case of an exception
        std::shared_lock<engine::SharedMutex> tmp_lock{event_mutex_, std::adopt_lock};

        // Now we want to create N subtasks for callbacks,
        // which must hold event_mutex_'s std::shared_lock.
        // A naive implementation would create std::shared_lock{event_mutex_} for each subtask,
        // however, it might deadlock if any parallel SendEvent() is called and is blocked on
        // event_mutex_.lock(). It happens due to strict prioritization of writers above readers
        // in SharedMutex: if there is any pending writer, nobody may lock the mutex for read.
        {
            auto data = data_.Lock();
            auto& listeners = data->listeners;
            tasks.reserve(listeners.size());

            for (const auto& [_, listener] : listeners) {
                tasks.push_back(Task{
                    listener,  // an intentional copy
                    utils::Async(
                        listener->task_name,
                        [&, &callback = listener->callback, sema_lock = std::shared_lock(listener->sema)] {
                            callback(args...);
                        }
                    ),
                });
            }
        }
        // Unlock data_ here because callbacks may subscribe to this

        for (auto& task : tasks) {
            impl::WaitForTask(task.listener->name, task.task);
        }
    }

    /// @returns the name of this event channel
    const std::string& Name() const noexcept { return name_; }

private:
    struct Listener final {
        // 'sema' with data_.Lock() are used to synchronize removal 'Listener' from ListenersData::listeners
        mutable engine::Semaphore sema;

        std::string name;
        Function callback;
        std::string task_name;

        Listener(std::string name, Function callback, std::string task_name)
            : sema(1),
              name(std::move(name)),
              callback(std::move(callback)),
              task_name(std::move(task_name))
        {}
    };

    struct ListenersData final {
        std::unordered_map<FunctionId, std::shared_ptr<const Listener>, FunctionId::Hash> listeners;
        OnRemoveCallback on_listener_removal;
    };

    void RemoveListener(FunctionId id, UnsubscribingKind kind) noexcept final {
        const engine::TaskCancellationBlocker blocker;
        const std::shared_lock lock(event_mutex_);
        std::shared_ptr<const Listener> listener;
        OnRemoveCallback on_listener_removal;

        {
            auto data = data_.Lock();
            auto& listeners = data->listeners;
            const auto iter = listeners.find(id);

            if (iter == listeners.end()) {
                impl::ReportNotSubscribed(Name());
                return;
            }

            listener = iter->second;

            on_listener_removal = data->on_listener_removal;

            listeners.erase(iter);

            // Lock and unlock sema under data_.Lock(),
            // now we're sure that SendEvent() will not trigger listener->callback()
            (void)std::shared_lock(listener->sema);
        }
        // Unlock data_ here to be able to (un)subscribe to *this in listener->callback (in debug)
        // without deadlock

        if (kind == UnsubscribingKind::kAutomatic) {
            if (!on_listener_removal) {
                impl::ReportUnsubscribingAutomatically(name_, listener->name);
            }

            if constexpr (impl::kCheckSubscriptionUB) {
                // Fake listener call to check
                impl::CheckDataUsedByCallbackHasNotBeenDestroyedBeforeUnsubscribing(
                    on_listener_removal,
                    listener->callback,
                    name_,
                    listener->name
                );
            }
        }
    }

    AsyncEventSubscriberScope DoAddListener(FunctionId id, std::string_view name, Function&& func) final {
        UASSERT(id);

        auto data = data_.Lock();
        auto& listeners = data->listeners;
        auto task_name = impl::MakeAsyncChannelName(name_, name);
        const auto [iterator, success] = listeners.emplace(
            id,
            std::make_shared<const Listener>(std::string{name}, std::move(func), std::move(task_name))
        );
        if (!success) {
            impl::ReportAlreadySubscribed(Name(), name);
        }
        return AsyncEventSubscriberScope(utils::impl::InternalTag{}, *this, id);
    }

    const std::string name_;
    concurrent::Variable<ListenersData> data_;

    // event_mutex_ is required only for event serialization,
    // it doesn't protect any data. The mutex is unique locked
    // for new event publishing, and is shared locked for calling callbacks.
    // If any callback is working, no new event publishing is possible.
    // It *is* possible to re-subscribe on async channel while another callback
    // operates.
    mutable engine::SharedMutex event_mutex_;
};

}  // namespace concurrent

USERVER_NAMESPACE_END
