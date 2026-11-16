#include <userver/concurrent/conflated_event_channel.hpp>
#include <userver/utils/async.hpp>

#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent {

ConflatedEventChannel::ConflatedEventChannel(std::string name, OnRemoveCallback on_listener_removal)
    : AsyncEventChannel<>(std::move(name), std::move(on_listener_removal)),
      data_(std::in_place, static_cast<AsyncEventChannel<>&>(*this))
{}

ConflatedEventChannel::~ConflatedEventChannel() = default;

ConflatedEventChannel::Data::Data(utils::ResourceScopeStorage& scopes, AsyncEventChannel<>& channel)
    : scopes(scopes),
      name(channel.Name())
{
    LOG_DEBUG() << name << ": start event listener task";
    task = utils::CriticalAsync(name + "/event_listener", [this, &channel] {
        while (event.WaitForEvent()) {
            if (stop_flag) {
                break;
            }
            LOG_TRACE() << name << ": send event";
            try {
                channel.SendEvent();
            } catch (const std::exception& e) {
                LOG_ERROR() << name << ": unexpected exception in SendEvent: " << e;
            }
        }
        LOG_DEBUG() << name << ": event listener task ends";
    });
}

ConflatedEventChannel::Data::~Data() {
    LOG_DEBUG() << name << ": stop event listener task";
    stop_flag = true;
    event.Send();
    try {
        task.Get();
    } catch (const std::exception& e) {
        LOG_ERROR() << name << ": unexpected exception in task.Get: " << e;
    }
}

void ConflatedEventChannel::Data::SendEvent() {
    LOG_TRACE() << name << ": channel event received";
    event.Send();
    if (task.IsFinished()) {
        throw std::runtime_error(name + ": event listener died unexpectedly");
    }
}

void ConflatedEventChannel::SendEvent() { data_->SendEvent(); }

}  // namespace concurrent

USERVER_NAMESPACE_END
