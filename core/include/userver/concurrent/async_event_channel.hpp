#pragma once

/// @file userver/concurrent/async_event_channel.hpp
/// @brief @copybrief concurrent::AsyncEventChannel

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
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
#include <userver/utils/resource_scopes.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent {

namespace impl {

struct AsyncEventChannelTestAccessor;

std::shared_lock<engine::SharedMutex> DowngradeLock(std::unique_lock<engine::SharedMutex> lock);

void WaitForTask(std::string_view name, engine::TaskWithResult<void>& task);

[[noreturn]] void ReportAlreadySubscribed(std::string_view channel_name, std::string_view listener_name);

void ReportNotSubscribed(std::string_view channel_name) noexcept;

void ReportErrorWhileUnsubscribing(
    std::string_view channel_name,
    std::string_view listener_name,
    std::string_view error
) noexcept;

std::string MakeAsyncChannelName(std::string_view base, std::string_view name);

inline constexpr bool kCheckSubscriptionUB = utils::impl::kEnableAssert;

// During `AsyncEventSubscriberScope::Unsubscribe` or automatic teardown
// (scope destructor / ResourceScopeStorage::BeforeDestruction), all variables
// used by the callback must still be valid. A common cause of crashes here:
// the subscription is removed after the captured data is destroyed.
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
/// @snippet core/src/concurrent/async_event_channel_test.cpp  AsyncEventChannel sample
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
    /// The constructor with a callback that is called on listener removal,
    /// both on `Unsubscribe` and on automatic teardown. The callback takes a
    /// reference to `Function` as input. This is useful for checking the lifetime
    /// of data captured by the listener update function.
    ///
    /// @note Works only in debug mode.
    ///
    /// @warning Data captured by `on_listener_removal` function must be valid
    /// until the `AsyncEventChannel` object is completely destroyed.
    ///
    /// Example usage:
    /// @snippet core/src/concurrent/async_event_channel_test.cpp OnListenerRemoval sample
    ///
    /// @param name used for diagnostic purposes and is also accessible with Name
    /// @param on_listener_removal the callback used for check
    ///
    /// @see impl::CheckDataUsedByCallbackHasNotBeenDestroyedBeforeUnsubscribing
    AsyncEventChannel(std::string_view name, OnRemoveCallback on_listener_removal)
        : name_(name),
          data_(ListenersData{{}, std::move(on_listener_removal)})
    {}

#ifndef ARCADIA_ROOT
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
#endif

    /// @brief Like @ref DoUpdateAndListen, but binds the subscription to @a scopes.
    ///
    /// Synchronously calls @a updater and records the channel's event generation. When the scope is entered,
    /// @a updater is called again if an event was sent during construction, then @a func is subscribed atomically
    /// with that check. Unsubscribes on scope exit.
    ///
    /// @warning @a updater is not only invoked inline. It is stored and may run again after this call returns,
    /// when the scope is entered. Do not capture locals by reference: copy the pointers and values the updater
    /// needs (for example @c obj and @c func) and capture @c this explicitly.
    ///
    /// @param scopes storage that owns the subscription lifetime.
    /// See @ref resource_scopes_parameter "where to obtain scopes".
    template <typename UpdaterFunc>
    void DoUpdateAndListen(
        utils::ResourceScopeStorage& scopes,
        FunctionId id,
        std::string_view name,
        Function&& func,
        UpdaterFunc&& updater
    ) {
        const auto initial_generation = [this, &updater] {
            const std::shared_lock lock(event_mutex_);
            updater();
            return event_generation_;
        }();

        // The first callback runs synchronously in the constructor, so the subscription
        // lifetime must be longer than that of other scope types.
        constexpr utils::ResourceScopeStorage::Priority kPriority{-1};
        scopes.Register(
            utils::impl::InternalTag{},
            kPriority,
            [this,
             id,
             name = std::string{name},
             initial_generation,
             func = std::move(func),
             updater = std::forward<UpdaterFunc>(updater)]() mutable {
                const std::shared_lock lock(event_mutex_);
                if (event_generation_ != initial_generation) {
                    updater();
                }
                return DoAddListener(id, name, std::move(func));
            }
        );
    }

    /// @overload
    /// @param scopes storage that owns the subscription lifetime.
    /// See @ref resource_scopes_parameter "where to obtain scopes".
    template <typename Class, typename UpdaterFunc>
    void DoUpdateAndListen(
        utils::ResourceScopeStorage& scopes,
        Class* obj,
        std::string_view name,
        void (Class::*func)(Args...),
        UpdaterFunc&& updater
    ) {
        DoUpdateAndListen(
            scopes,
            FunctionId(obj),
            name,
            [obj, func](Args... args) { (obj->*func)(args...); },
            std::forward<UpdaterFunc>(updater)
        );
    }

#ifndef ARCADIA_ROOT

    /// @deprecated Use @ref concurrent::AsyncEventChannel::DoUpdateAndListen instead.
    template <typename UpdaterFunc>
    void DoUpdateAndListenScoped(
        utils::ResourceScopeStorage& scopes,
        FunctionId id,
        std::string_view name,
        Function&& func,
        UpdaterFunc&& updater
    ) {
        DoUpdateAndListen(scopes, id, name, std::move(func), std::forward<UpdaterFunc>(updater));
    }

    /// @deprecated Use @ref concurrent::AsyncEventChannel::DoUpdateAndListen instead.
    template <typename Class, typename UpdaterFunc>
    void DoUpdateAndListenScoped(
        utils::ResourceScopeStorage& scopes,
        Class* obj,
        std::string_view name,
        void (Class::*func)(Args...),
        UpdaterFunc&& updater
    ) {
        DoUpdateAndListen(scopes, obj, name, func, std::forward<UpdaterFunc>(updater));
    }

#endif

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

        std::shared_lock<engine::SharedMutex> tmp_lock;
        {
            // Try to obtain unique lock for event_mutex_ to serialize
            // calls to SendEvent()
            std::unique_lock lock(event_mutex_);
            auto data = data_.Lock();
            ++event_generation_;

            tmp_lock = impl::DowngradeLock(std::move(lock));

            // Now we want to create N subtasks for callbacks,
            // which must hold event_mutex_'s std::shared_lock.
            // A naive implementation would create std::shared_lock{event_mutex_} for each subtask,
            // however, it might deadlock if any parallel SendEvent() is called and is blocked on
            // event_mutex_.lock(). It happens due to strict prioritization of writers above readers
            // in SharedMutex: if there is any pending writer, nobody may lock the mutex for read.
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
    friend struct impl::AsyncEventChannelTestAccessor;

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

    void RemoveListener(FunctionId id, [[maybe_unused]] UnsubscribingKind kind) noexcept final {
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

    // The mutex is unique locked
    // for new event publishing, and is shared locked for calling callbacks.
    // If any callback is working, no new event publishing is possible.
    // It *is* possible to re-subscribe on async channel while another callback
    // operates.
    mutable engine::SharedMutex event_mutex_;
    mutable std::uint64_t event_generation_{0};
};

}  // namespace concurrent

USERVER_NAMESPACE_END
