#include <userver/storages/sqlite/infra/strategy/pool_strategy.hpp>

#include <userver/engine/async.hpp>
#include <userver/engine/wait_all_checked.hpp>

#include <userver/storages/sqlite/infra/pool.hpp>
#include <userver/storages/sqlite/infra/strategy/exclusive_read_write.hpp>
#include <userver/storages/sqlite/infra/strategy/read_only.hpp>
#include <userver/storages/sqlite/infra/strategy/read_write.hpp>
#include <userver/storages/sqlite/options.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::infra::strategy {

PoolStrategyBase::~PoolStrategyBase() = default;

std::unique_ptr<PoolStrategyBase>
PoolStrategyBase::Create(const settings::SQLiteSettings& settings, engine::TaskProcessor& blocking_task_processor) {
    if (settings.read_mode == settings::SQLiteSettings::ReadMode::kReadOnly) {
        // many readers and null writers in one time
        return std::make_unique<ReadOnlyStrategy>(settings, blocking_task_processor);
    } else if (settings.journal_mode == settings::SQLiteSettings::JournalMode::kWal) {
        // many readers and one writer in one time
        return std::make_unique<ReadWriteStrategy>(settings, blocking_task_processor);
    } else {
        // in case of rollback journal it need more advanced pool strategy with
        // waiting on pools iternal state or sqlite lock state
        // many readers or one writer in one time
        return std::make_unique<ExclusiveReadWriteStrategy>(settings, blocking_task_processor);
    }
}

Pool& PoolStrategyBase::SelectPool(OperationType operation_type) const {
    switch (operation_type) {
        case OperationType::kReadOnly:
            return GetReadOnly();
        case OperationType::kReadWrite:
            return GetReadWrite();
    }

    UINVARIANT(false, "Unknown host type");
}

}  // namespace storages::sqlite::infra::strategy

USERVER_NAMESPACE_END
