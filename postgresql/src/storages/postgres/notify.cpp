#include <userver/storages/postgres/notify.hpp>

#include <storages/postgres/detail/connection.hpp>
#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

struct NotifyScope::Impl {
    detail::ConnectionPtr conn;
    std::string channel;
    OptionalCommandControl cmd_ctl;

    Impl(detail::ConnectionPtr conn, std::string_view channel, OptionalCommandControl cmd_ctl)
        : conn{std::move(conn)}, channel{channel}, cmd_ctl{cmd_ctl} {
        Listen();
    }

    ~Impl() { Unlisten(); }

    Impl(Impl&&) = default;
    Impl& operator=(Impl&&) = default;

    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;

    Notification WaitNotify(engine::Deadline deadline) {
        UINVARIANT(conn, "Called WaitNotify on empty NotifyScope");
        return conn->WaitNotify(deadline);
    }

private:
    void Listen() {
        UASSERT(conn);
        LOG_DEBUG() << "Start listening on channel '" << channel << "'";
        conn->Listen(channel, cmd_ctl);
    }

    void Unlisten() {
        if (!conn) return;
        try {
            LOG_DEBUG() << "Stop listening on channel '" << channel << "'";
            conn->Unlisten(channel, cmd_ctl);
        } catch (const std::exception& e) {
            LOG_LIMITED_ERROR() << "Exception while executing unlisten: " << e;
            // Will be closed to avoid unsolicited notifications in the future
            conn->MarkAsBroken();
        }
    }
};

NotifyScope::NotifyScope(detail::ConnectionPtr conn, std::string_view channel, OptionalCommandControl cmd_ctl)
    : pimpl_{std::move(conn), channel, cmd_ctl} {}

NotifyScope::~NotifyScope() = default;

NotifyScope::NotifyScope(NotifyScope&&) noexcept = default;

NotifyScope& NotifyScope::operator=(NotifyScope&&) noexcept = default;

Notification NotifyScope::WaitNotify(engine::Deadline deadline) { return pimpl_->WaitNotify(deadline); }

}  // namespace storages::postgres

USERVER_NAMESPACE_END
