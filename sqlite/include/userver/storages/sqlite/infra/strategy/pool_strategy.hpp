#pragma once

#include <memory>

#include <userver/engine/async.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/statistics/writer.hpp>

#include <userver/storages/sqlite/operation_types.hpp>
#include <userver/storages/sqlite/options.hpp>
#include <userver/storages/sqlite/sqlite_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::infra::strategy {

class PoolStrategyBase {
public:
    virtual ~PoolStrategyBase();

    static std::unique_ptr<PoolStrategyBase>
    Create(const settings::SQLiteSettings& settings, engine::TaskProcessor& blocking_task_processor);

    Pool& SelectPool(OperationType op_type) const;

    virtual void WriteStatistics(utils::statistics::Writer& writer) const = 0;

protected:
    virtual Pool& GetReadOnly() const = 0;
    virtual Pool& GetReadWrite() const = 0;
};

}  // namespace storages::sqlite::infra::strategy

USERVER_NAMESPACE_END
