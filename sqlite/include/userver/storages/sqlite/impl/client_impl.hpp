#pragma once

/// @file userver/storages/sqlite/connection.hpp
/// @copybrief @copybrief storages::sqlite::Connection

#include <memory>

#include <userver/engine/task/task_processor_fwd.hpp>

#include <userver/storages/sqlite/impl/statements_base.hpp>
#include <userver/storages/sqlite/options.hpp>
#include <userver/storages/sqlite/query.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

namespace infra {

class ConnectionPtr;

namespace strategy {
class PoolStrategyBase;
using PoolStrategyBasePtr = std::unique_ptr<PoolStrategyBase>;
}  // namespace strategy

}  // namespace infra

namespace impl {

class Statement;
using StatementPtr = std::shared_ptr<Statement>;

class ClientImpl;
using ClientImplPtr = std::unique_ptr<ClientImpl>;

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

}  // namespace impl

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
