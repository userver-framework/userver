#pragma once

#include <memory>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/drivers/impl/connection_pool_base.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/utils/datetime/wall_coarse_clock.hpp>
#include <userver/utils/periodic_task.hpp>
#include <userver/utils/statistics/relaxed_counter.hpp>

#include <userver/storages/sqlite/impl/connection.hpp>
#include <userver/storages/sqlite/infra/connection_ptr.hpp>
#include <userver/storages/sqlite/options.hpp>
#include <userver/storages/sqlite/sqlite_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::infra {

using Counter = utils::statistics::RelaxedCounter<std::uint64_t>;

struct PoolConnectionStatistics final {
  Counter overload{};
  Counter closed{};
  Counter created{};
  Counter acquired{};
  Counter released{};
};

void DumpMetric(utils::statistics::Writer& writer,
                const PoolConnectionStatistics& stats);

class Pool final
    : public drivers::impl::ConnectionPoolBase<impl::Connection, Pool> {
 public:
  static std::shared_ptr<Pool> Create(
      const settings::SQLiteSettings& settings,
      engine::TaskProcessor& blocking_task_processor);
  ~Pool();

  ConnectionPtr Acquire();
  void Release(ConnectionUniquePtr connection);

  Pool(const settings::SQLiteSettings& settings,
       engine::TaskProcessor& blocking_task_processor);

  // It's possible to use in advanced exclusive read write connection pools
  // strategy
  Counter GetCurrentWorkersCount() const;

 private:
  friend class drivers::impl::ConnectionPoolBase<impl::Connection, Pool>;

  ConnectionUniquePtr DoCreateConnection(engine::Deadline deadline);

  void AccountConnectionAcquired();
  void AccountConnectionReleased();
  void AccountConnectionCreated();
  void AccountConnectionDestroyed() noexcept;
  void AccountOverload();

  engine::TaskProcessor& blocking_task_processor_;

  const settings::SQLiteSettings settings_;

  PoolConnectionStatistics stats_{};
};

}  // namespace storages::sqlite::infra

USERVER_NAMESPACE_END
