#pragma once

/// @file userver/concurrent/conflated_event_channel.hpp
/// @brief @copybrief concurrent::ConflatedEventChannel

#include <atomic>
#include <string>
#include <utility>

#include <userver/concurrent/async_event_channel.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/utils/resource_scopes.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent {

/// @ingroup userver_concurrency
///
/// @brief A non-blocking version of 'AsyncEventChannel'
///
/// The difference is that 'SendEvent' returns immediately, without waiting for
/// subscribers to finish. If 'SendEvent' is called multiple times while
/// subscribers are handling the previous event, new events will be conflated
/// (all events except for the last one will be ignored). This class can be used
/// instead of 'AsyncEventChannel' when we've got a "heavy" subscriber and we
/// don't want to slow down the pipeline.
class ConflatedEventChannel : private AsyncEventChannel<> {
public:
    explicit ConflatedEventChannel(std::string name, OnRemoveCallback on_listener_removal = {});
    ~ConflatedEventChannel() override;

    /// @brief Forwards new events from an @ref concurrent::AsyncEventSource without replaying past events.
    template <typename... Args>
    void AddChannel(concurrent::AsyncEventSource<Args...>& channel);

    using AsyncEventChannel<>::AddListener;

    /// Subscribes to updates using a member function. Also immediately invokes
    /// the function with the current config snapshot.
    ///
    /// Further updates are delivered after @ref utils::ResourceScopeStorage::AfterConstruction, including those updates
    /// that arrived before it. Unsubscribe runs in @ref utils::ResourceScopeStorage::BeforeDestruction.
    ///
    /// @param scopes storage that owns the subscription lifetime.
    /// See @ref resource_scopes_parameter "where to obtain scopes".
    template <typename Class>
    void UpdateAndListen(utils::ResourceScopeStorage& scopes, Class* obj, std::string_view name, void (Class::*func)());

#ifndef ARCADIA_ROOT
    /// @overload
    /// @deprecated Use the overload that takes @ref utils::ResourceScopeStorage.
    ///
    /// Store the returned scope as a member and call `Unsubscribe` explicitly.
    template <typename Class>
    concurrent::AsyncEventSubscriberScope UpdateAndListen(Class* obj, std::string_view name, void (Class::*func)());
#endif

    void SendEvent();

private:
    struct Data final {
        Data(utils::ResourceScopeStorage& scopes, AsyncEventChannel<>& channel);
        ~Data();

        void SendEvent();

        template <typename... Args>
        void OnChannelEvent(Args...) {
            SendEvent();
        }

        utils::ResourceScopeStorage& scopes;
        const std::string name;
        std::atomic<bool> stop_flag{false};
        engine::SingleConsumerEvent event;
        engine::TaskWithResult<void> task;
    };

    utils::WithResourceScopes<Data> data_;
};

template <typename... Args>
void ConflatedEventChannel::AddChannel(concurrent::AsyncEventSource<Args...>& channel) {
    auto& data = *data_;
    channel.AddListener(data.scopes, &data, data.name, &Data::OnChannelEvent<Args...>);
}

template <typename Class>
void ConflatedEventChannel::UpdateAndListen(
    utils::ResourceScopeStorage& scopes,
    Class* obj,
    std::string_view name,
    void (Class::*func)()
) {
    DoUpdateAndListen(scopes, obj, name, func, [obj, func] { (obj->*func)(); });
}

#ifndef ARCADIA_ROOT
template <typename Class>
concurrent::AsyncEventSubscriberScope ConflatedEventChannel::UpdateAndListen(
    Class* obj,
    std::string_view name,
    void (Class::*func)()
) {
    return DoUpdateAndListen(obj, name, func, [&] { (obj->*func)(); });
}
#endif

}  // namespace concurrent

USERVER_NAMESPACE_END
