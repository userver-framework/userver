#include <userver/storages/sqlite/component.hpp>

#include <memory>

#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/logging/log.hpp>
#include <userver/testsuite/tasks.hpp>
#include <userver/utils/assert.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/yaml_config/schema.hpp>

#include <userver/storages/sqlite/client.hpp>
#include <userver/storages/sqlite/options.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

namespace {

storages::sqlite::settings::SQLiteSettings::JournalMode ParseJournalMode(const std::string& value) {
    if (value == "delete") return storages::sqlite::settings::SQLiteSettings::JournalMode::kDelete;
    if (value == "truncate") return storages::sqlite::settings::SQLiteSettings::JournalMode::kTruncate;
    if (value == "persist") return storages::sqlite::settings::SQLiteSettings::JournalMode::kPersist;
    if (value == "memory") return storages::sqlite::settings::SQLiteSettings::JournalMode::kMemory;
    if (value == "wal") return storages::sqlite::settings::SQLiteSettings::JournalMode::kWal;
    if (value == "off") return storages::sqlite::settings::SQLiteSettings::JournalMode::kOff;
    UINVARIANT(false, "Unknown journal mode: " + value);
}

storages::sqlite::settings::SQLiteSettings::Synchronous ParseSynchronous(const std::string& value) {
    if (value == "extra") return storages::sqlite::settings::SQLiteSettings::Synchronous::kExtra;
    if (value == "full") return storages::sqlite::settings::SQLiteSettings::Synchronous::kFull;
    if (value == "normal") return storages::sqlite::settings::SQLiteSettings::Synchronous::kNormal;
    if (value == "off") return storages::sqlite::settings::SQLiteSettings::Synchronous::kOff;
    UINVARIANT(false, "Unknown synchronous: " + value);
}

storages::sqlite::settings::SQLiteSettings::TempStore ParseTempStore(const std::string& value) {
    if (value == "memory") return storages::sqlite::settings::SQLiteSettings::TempStore::kMemory;
    if (value == "file") return storages::sqlite::settings::SQLiteSettings::TempStore::kFile;
    UINVARIANT(false, "Unknown synchronous: " + value);
}

storages::sqlite::settings::SQLiteSettings GetSettings(const components::ComponentConfig& config) {
    storages::sqlite::settings::SQLiteSettings settings;
    settings.db_path = config["db-path"].As<std::string>();
    settings.create_file = config["create_file"].As<bool>(settings.create_file);
    settings.read_mode = config["is_read_only"].As<bool>(storages::sqlite::settings::kDefaultIsReadOnly)
                             ? storages::sqlite::settings::SQLiteSettings::ReadMode::kReadOnly
                             : storages::sqlite::settings::SQLiteSettings::ReadMode::kReadWrite;
    settings.shared_cache = config["shared_cache"].As<bool>(settings.shared_cache);
    settings.read_uncommitted = config["read_uncommitted"].As<bool>(settings.read_uncommitted);
    settings.foreign_keys = config["foreign_keys"].As<bool>(settings.foreign_keys);
    settings.journal_mode =
        ParseJournalMode(config["journal_mode"].As<std::string>(storages::sqlite::settings::kDefaultJournalMode));
    settings.synchronous =
        ParseSynchronous(config["synchronous"].As<std::string>(storages::sqlite::settings::kDefaultSynchronous));
    settings.temp_store =
        ParseTempStore(config["temp_store"].As<std::string>(storages::sqlite::settings::kDefaultTempStore));
    settings.busy_timeout = config["busy_timeout"].As<int>(settings.busy_timeout);
    settings.cache_size = config["cache_size"].As<int>(settings.cache_size);
    settings.journal_size_limit = config["journal_size_limit"].As<int>(settings.journal_size_limit);
    settings.mmap_size = config["mmap_size"].As<int>(settings.mmap_size);
    settings.page_size = config["page_size"].As<int>(settings.page_size);
    settings.conn_settings = storages::sqlite::settings::ConnectionSettings::Create(config);
    settings.pool_settings = storages::sqlite::settings::PoolSettings::Create(config);

    return settings;
}

}  // namespace

SQLite::SQLite(const ComponentConfig& config, const ComponentContext& context)
    : ComponentBase{config, context},
      settings_(GetSettings(config)),
      fs_task_processor_(GetFsTaskProcessor(config, context)),
      client_(std::make_shared<storages::sqlite::Client>(settings_, fs_task_processor_)) {
    auto& statistics_storage = context.FindComponent<components::StatisticsStorage>();
    statistics_holder_ = statistics_storage.GetStorage().RegisterWriter(
        "sqlite",
        [this](utils::statistics::Writer& writer) { client_->WriteStatistics(writer); },
        {{"component", config.Name()}}
    );

    auto& testsuite_tasks = testsuite::GetTestsuiteTasks(context);
    if (testsuite_tasks.IsEnabled()) {
        // Only register task for testsuite environment
        const auto task_name = fmt::format("sqlite/{}/clean-db", config.Name());
        testsuite_tasks.RegisterTask(task_name, [this] {
            engine::TaskCancellationBlocker block_cancel;
            const auto table_names =
                client_
                    ->Execute(
                        storages::sqlite::OperationType::kReadOnly,
                        "select name from sqlite_schema where type == 'table' AND name NOT LIKE 'sqlite_%'"
                    )
                    .AsVector<std::string>(storages::sqlite::kFieldTag);
            for (const auto& table : table_names) {
                client_->Execute(storages::sqlite::OperationType::kReadWrite, fmt::format("delete from '{}'", table));
            }
        });
    }
}

SQLite::~SQLite() { statistics_holder_.Unregister(); }

storages::sqlite::ClientPtr SQLite::GetClient() const { return client_; }

yaml_config::Schema SQLite::GetStaticConfigSchema() {
    return yaml_config::MergeSchemas<ComponentBase>(R"(
type: object
description: SQLite client component
additionalProperties: false
properties:
    fs-task-processor:
        type: string
        description: name of the task processor to handle the blocking file operations
        defaultDescription: engine::current_task::GetBlockingTaskProcessor()
    db-path:
        type: string
        description: path to the database file or `::memory::` for in-memory mode
    create_file:
        type: boolean
        description: сreate the database file if it does not exist at the specified path
        defaultDescription: true
    is_read_only:
        type: boolean
        description: open the database in read-only mode
        defaultDescription: false
    shared_cache:
        type: boolean
        description: enable shared in-memory cache for the database
        defaultDescription: false
    read_uncommitted:
        type: boolean
        description: allow reading uncommitted data (requires shared_cache)
        defaultDescription: false
    journal_mode:
        type: string
        description: mode for database journaling
        defaultDescription: wal
        enum:
          - delete
          - truncate
          - persist
          - memory
          - wal
          - off
    busy_timeout:
        type: integer
        description: timeout duration (in milliseconds) to wait when the database is busy
        defaultDescription: 5000
    foreign_keys:
        type: boolean
        description: enable enforcement of foreign key constraints
        defaultDescription: true
    synchronous:
        type: string
        description: set the level of synchronization to ensure data durability
        defaultDescription: normal
        enum:
          - extra
          - full
          - normal
          - off
    cache_size:
        type: integer
        description: maximum cache size, specified in number of pages or in kibibytes (negative value)
        defaultDescription: -2000
    journal_size_limit:
        type: integer
        description: limit the size of rollback-journal and WAL files (in bytes)
        defaultDescription: 67108864
    mmap_size:
        type: integer
        description: maximum number of bytes allocated for memory-mapped I/O
        defaultDescription: 30000000000
    page_size:
        type: integer
        description: size of a database page (in bytes)
        defaultDescription: 4096
    temp_store:
        type: string
        description: storage location for temporary tables and indexes
        defaultDescription: memory
        enum:
          - memory
          - file
    persistent-prepared-statements:
        type: boolean
        description: cache prepared statements for reuse
        defaultDescription: true
    max_prepared_cache_size:
        type: integer
        description: maximum number of prepared statements to cache
        defaultDescription: 200
    initial_read_only_pool_size:
        type: integer
        description: initial size of the read-only connection pool
        defaultDescription: 5
    max_read_only_pool_size:
        type: integer
        description: maximum size of the read-only connection pool
        defaultDescription: 10
)");
}

}  // namespace components

USERVER_NAMESPACE_END
