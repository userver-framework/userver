#pragma once

/// @file userver/os_signals/processor.hpp
/// @brief @copybrief os_signals::Processor

#include <userver/concurrent/async_event_channel.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/os_signals/subscriber.hpp>
#include <userver/utils/impl/internal_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace os_signals {

#if !defined(__APPLE__)

/// @brief Constant for SIGUSR1
inline constexpr int kSigUsr1 = 10;

/// @brief Constant for SIGUSR2
inline constexpr int kSigUsr2 = 12;

#else
inline constexpr int kSigUsr1 = 30;
inline constexpr int kSigUsr2 = 31;
#endif

/// @ingroup userver_clients
///
/// @brief A client that allows you to subscribe
/// to OS signals `SIGUSR1` and `SIGUSR2`.
///
/// Usually retrieved from os_signals::ProcessorComponent. For tests
/// use os_signals::ProcessorMock.
///
/// @see @ref scripts/docs/en/userver/os_signals.md
class Processor final {
public:
    explicit Processor(engine::TaskProcessor& task_processor);

    /// @brief Listens for a specific signal during the resource scope lifetime.
    /// @param scopes storage that owns the subscription lifetime.
    /// See @ref resource_scopes_parameter "where to obtain scopes".
    template <class Class>
    void AddListener(
        utils::ResourceScopeStorage& scopes,
        Class* obj,
        std::string_view name,
        int signum,
        void (Class::*func)()
    ) {
        channel_.AddListener(scopes, concurrent::FunctionId(obj), name, [obj, func, signum](int sig) {
            if (sig == signum) {
                (obj->*func)();
            }
        });
    }

    /// @brief Listens for all OS signals during the resource scope lifetime.
    /// @param scopes storage that owns the subscription lifetime.
    /// See @ref resource_scopes_parameter "where to obtain scopes".
    template <class Class>
    void AddListener(utils::ResourceScopeStorage& scopes, Class* obj, std::string_view name, void (Class::*func)(int)) {
        channel_.AddListener(scopes, obj, name, func);
    }

    /// Listen for a specific signal `signum`. See the other overload for
    /// subscription to both signals.
    /// @deprecated Use the overload that takes @ref utils::ResourceScopeStorage.
    template <class Class>
    Subscriber AddListener(Class* obj, std::string_view name, int signum, void (Class::*func)()) {
        auto execute = [obj, func, signum](int sig) {
            if (sig == signum) {
                (obj->*func)();
            }
        };

        return channel_.AddListener(concurrent::FunctionId(obj), name, std::move(execute));
    }

    /// Listen for all the OS signals.
    /// @deprecated Use the overload that takes @ref utils::ResourceScopeStorage.
    template <class Class>
    Subscriber AddListener(Class* obj, std::string_view name, void (Class::*func)(int /*signum*/)) {
        auto execute = [obj, func](int sig) { (obj->*func)(sig); };
        return channel_.AddListener(concurrent::FunctionId(obj), name, std::move(execute));
    }

    /// @cond
    // For internal use
    void Notify(int signum, utils::impl::InternalTag);
    /// @endcond
private:
    concurrent::AsyncEventChannel<int> channel_;
    engine::TaskProcessor& task_processor_;
};

}  // namespace os_signals

USERVER_NAMESPACE_END
