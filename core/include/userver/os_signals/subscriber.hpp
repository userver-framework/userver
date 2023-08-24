#pragma once

/// @file userver/os_signals/subscriber.hpp
/// @brief @copybrief components::os_signals::Subscriber

#include <userver/concurrent/async_event_source.hpp>

USERVER_NAMESPACE_BEGIN

namespace os_signals {

/// @see concurrent::AsyncEventSubscriberScope
using Subscriber = concurrent::AsyncEventSubscriberScope;

}  // namespace os_signals

USERVER_NAMESPACE_END
