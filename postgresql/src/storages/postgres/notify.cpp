#include <userver/storages/postgres/notify.hpp>

#include <storages/postgres/detail/connection.hpp>
#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

struct NotifyScope::Impl {
  detail::ConnectionPtr conn_;
  std::string channel_;
  OptionalCommandControl cmd_ctl_;

  Impl(detail::ConnectionPtr conn, std::string_view channel,
       OptionalCommandControl cmd_ctl)
      : conn_{std::move(conn)}, channel_{channel}, cmd_ctl_{cmd_ctl} {
    Listen();
  }

  ~Impl() { Unlisten(); }

  Impl(Impl&&) = default;
  Impl& operator=(Impl&&) = default;

  Impl(const Impl&) = delete;
  Impl& operator=(const Impl&) = delete;

  Notification WaitNotify(engine::Deadline deadline) {
    UINVARIANT(conn_, "Called WaitNotify on empty NotifyScope");
    return conn_->WaitNotify(deadline);
  }

 private:
  void Listen() {
    UASSERT(conn_);
    LOG_DEBUG() << "Start listening on channel '" << channel_ << "'";
    conn_->Listen(channel_, cmd_ctl_);
  }

  void Unlisten() {
    if (!conn_) return;
    try {
      LOG_DEBUG() << "Stop listening on channel '" << channel_ << "'";
      conn_->Unlisten(channel_, cmd_ctl_);
    } catch (const std::exception& e) {
      LOG_LIMITED_ERROR() << "Exception while executing unlisten: " << e;
      // Will be closed to avoid unsolicited notifications in the future
      conn_->MarkAsBroken();
    }
  }
};

NotifyScope::NotifyScope(detail::ConnectionPtr conn, std::string_view channel,
                         OptionalCommandControl cmd_ctl)
    : pimpl_{std::move(conn), channel, cmd_ctl} {}

NotifyScope::~NotifyScope() = default;

NotifyScope::NotifyScope(NotifyScope&&) noexcept = default;

NotifyScope& NotifyScope::operator=(NotifyScope&&) noexcept = default;

Notification NotifyScope::WaitNotify(engine::Deadline deadline) {
  return pimpl_->WaitNotify(deadline);
}

}  // namespace storages::postgres

USERVER_NAMESPACE_END
