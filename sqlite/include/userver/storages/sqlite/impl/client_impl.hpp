#pragma once

/// @file userver/storages/sqlite/connection.hpp
/// @copybrief @copybrief storages::sqlite::Connection

#include <userver/engine/task/task_processor_fwd.hpp>

#include <userver/storages/sqlite/options.hpp>
#include <userver/storages/sqlite/query.hpp>
#include <userver/storages/sqlite/sqlite_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

class ClientImpl final {
 public:
  ClientImpl(const settings::SQLiteSettings& settings,
             engine::TaskProcessor& blocking_task_processor);
  ~ClientImpl();

  infra::ConnectionPtr GetConnection(
      settings::CommandControl::OperationType op_type) const;

  impl::StatementBasePtr PrepareStatement(
      const Query& query, infra::ConnectionPtr& connection) const;

 private:
  infra::strategy::PoolStrategyBasePtr pool_strategy_;
};

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
