#pragma once

#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/statistics/writer.hpp>

#include <userver/storages/sqlite/impl/io/params_binder_base.hpp>
#include <userver/storages/sqlite/operation_types.hpp>
#include <userver/storages/sqlite/options.hpp>
#include <userver/storages/sqlite/query.hpp>
#include <userver/storages/sqlite/sqlite_fwd.hpp>
#include "userver/storages/sqlite/result_set.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

class ClientImpl final {
public:
    ClientImpl(const settings::SQLiteSettings& settings, engine::TaskProcessor& blocking_task_processor);
    ~ClientImpl();

    std::shared_ptr<infra::ConnectionPtr> GetConnection(OperationType op_type) const;

    void WriteStatistics(utils::statistics::Writer& writer) const;

    ResultSet ExecuteCommand(
        impl::StatementBasePtr prepare_statement,
        std::shared_ptr<infra::ConnectionPtr> connection_ptr
    ) const;

    void AccountQueryExecute(std::shared_ptr<infra::ConnectionPtr> connection) const noexcept;
    void AccountQueryFailed(std::shared_ptr<infra::ConnectionPtr> connection) const noexcept;

private:
    infra::strategy::PoolStrategyBasePtr pool_strategy_;
};

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
