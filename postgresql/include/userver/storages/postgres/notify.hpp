#pragma once

/// @file userver/storages/postgres/notify.hpp
/// @brief Asynchronous notifications

#include <userver/storages/postgres/options.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

namespace detail {
class ConnectionPtr;
}

struct Notification {
  std::string channel;
  std::optional<std::string> payload;
};

/// @brief RAII scope for receiving notifications.
///
/// Used for waiting for notifications on PostgreSQL connections. Created by
/// calling storages::postgres::Cluster::Listen(). Exclusively holds a
/// connection from a pool.
///
/// Non-copyable.
///
/// @par Usage synopsis
/// @code
/// auto scope = cluster.Listen("channel");
/// cluster.Execute(pg::ClusterHostType::kMaster,
///                 "select pg_notify('channel', NULL)");
/// auto ntf = scope.WaitNotify(engine::Deadline::FromDuration(100ms));
/// @endcode
class [[nodiscard]] NotifyScope final {
 public:
  NotifyScope(detail::ConnectionPtr conn, std::string_view channel,
              OptionalCommandControl cmd_ctl);

  ~NotifyScope();

  NotifyScope(NotifyScope&&) noexcept;
  NotifyScope& operator=(NotifyScope&&) noexcept;

  NotifyScope(const NotifyScope&) = delete;
  NotifyScope& operator=(const NotifyScope&) = delete;

  /// Wait for notification on connection
  Notification WaitNotify(engine::Deadline deadline);

 private:
  struct Impl;
  USERVER_NAMESPACE::utils::FastPimpl<Impl, 80, 8> pimpl_;
};

}  // namespace storages::postgres

USERVER_NAMESPACE_END
