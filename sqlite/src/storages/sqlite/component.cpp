#include <userver/storages/sqlite/component.hpp>

#include <memory>

#include <userver/components/component.hpp>
#include <userver/utils/assert.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/yaml_config/schema.hpp>

#include <userver/storages/sqlite/client.hpp>
#include <userver/storages/sqlite/options.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

namespace {

storages::sqlite::settings::SQLiteSettings::JournalMode ParseJournalMode(
    const std::string& value) {
  if (value == "delete")
    return storages::sqlite::settings::SQLiteSettings::JournalMode::kDelete;
  if (value == "truncate")
    return storages::sqlite::settings::SQLiteSettings::JournalMode::kTruncate;
  if (value == "persist")
    return storages::sqlite::settings::SQLiteSettings::JournalMode::kPersist;
  if (value == "memory")
    return storages::sqlite::settings::SQLiteSettings::JournalMode::kMemory;
  if (value == "wal")
    return storages::sqlite::settings::SQLiteSettings::JournalMode::kWal;
  if (value == "off")
    return storages::sqlite::settings::SQLiteSettings::JournalMode::kOff;
  UINVARIANT(false, "Unknown journal mode: " + value);
}

storages::sqlite::settings::SQLiteSettings::Synchronous ParseSynchronous(
    const std::string& value) {
  if (value == "extra")
    return storages::sqlite::settings::SQLiteSettings::Synchronous::kExtra;
  if (value == "full")
    return storages::sqlite::settings::SQLiteSettings::Synchronous::kFull;
  if (value == "normal")
    return storages::sqlite::settings::SQLiteSettings::Synchronous::kNormal;
  if (value == "off")
    return storages::sqlite::settings::SQLiteSettings::Synchronous::kOff;
  UINVARIANT(false, "Unknown synchronous: " + value);
}

storages::sqlite::settings::SQLiteSettings::TempStore ParseTempStore(
    const std::string& value) {
  if (value == "memory")
    return storages::sqlite::settings::SQLiteSettings::TempStore::kMemory;
  if (value == "file")
    return storages::sqlite::settings::SQLiteSettings::TempStore::kFile;
  UINVARIANT(false, "Unknown synchronous: " + value);
}

std::shared_ptr<storages::sqlite::Client> CreateClient(
    const components::ComponentConfig& config,
    const components::ComponentContext& context) {
  storages::sqlite::settings::SQLiteSettings settings;
  settings.db_name = config["db-path"].As<std::string>();
  settings.create_file = config["create_file"].As<bool>(settings.create_file);
  settings.read_mode =
      config["is_read_only"].As<bool>(
          storages::sqlite::settings::kDefaultIsReadOnly)
          ? storages::sqlite::settings::SQLiteSettings::ReadMode::kReadOnly
          : storages::sqlite::settings::SQLiteSettings::ReadMode::kReadWrite;
  settings.shared_cashe =
      config["shared_cashe"].As<bool>(settings.shared_cashe);
  settings.shared_cashe =
      config["foreign_keys"].As<bool>(settings.foreign_keys);
  settings.journal_mode =
      ParseJournalMode(config["journal_mode"].As<std::string>(
          storages::sqlite::settings::kDefaultJournalMode));
  settings.synchronous = ParseSynchronous(config["synchronous"].As<std::string>(
      storages::sqlite::settings::kDefaultSynchronous));
  settings.temp_store = ParseTempStore(config["temp_store"].As<std::string>(
      storages::sqlite::settings::kDefaultTempStore));
  settings.busy_timeout = config["busy_timeout"].As<int>(settings.busy_timeout);
  settings.cache_size = config["cache_size"].As<int>(settings.cache_size);
  settings.journal_size_limit =
      config["journal_size_limit"].As<int>(settings.journal_size_limit);
  settings.mmap_size = config["mmap_size"].As<int>(settings.mmap_size);
  settings.page_size = config["page_size"].As<int>(settings.page_size);
  settings.conn_settings =
      storages::sqlite::settings::ConnectionSettings::Create(config);
  settings.pool_settings =
      storages::sqlite::settings::PoolSettings::Create(config);
  return std::make_shared<storages::sqlite::Client>(
      settings,
      context.GetTaskProcessor(config["task_processor"].As<std::string>()));
}

}  // namespace

SQLite::SQLite(const ComponentConfig& config, const ComponentContext& context)
    : ComponentBase{config, context},
      name_{config.Name()},
      client_(CreateClient(config, context)) {}

SQLite::~SQLite() = default;

storages::sqlite::ClientPtr SQLite::GetClient() const { return client_; }

yaml_config::Schema SQLite::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<ComponentBase>(R"(
type: object
description: SQLite client component
additionalProperties: false
properties:
    task_processor:
        type: string
        description: name of the task processor to run the blocking file operations
    db-path:
        type: string
        description: path to database file or `::memory` for in-memory mode
    create_file:
        type: boolean
        description: create a file if one is not found along the db-path
        defaultDescription: true
    is_read_only:
        type: boolean
        description: defines database access as read-only
    shared_cashe:
        type: boolean
        description: open database with shared in-memory cashe
        defaultDescription: false
    journal_mode:
        type: string
        description: journal mode
        defaultDescription: wal
        enum:
          - delete
          - truncate
          - persist
          - memory
          - wal
          - off
    synchronous:
        type: string
        description: durability level
        defaultDescription: normal
        enum:
          - extra
          - full
          - normal
          - off
    temp_store:
        type: string
        description: where temporary tables and indices are stored
        defaultDescription: memory
        enum:
          - memory
          - file
    busy_timeout:
        type: integer
        description: queries busy timeout
        defaultDescription: 0
    foreign_keys:
        type: boolean
        description: enable foreign keys
        defaultDescription: true
    cache_size:
        type: integer
        description: maximum cache size. In page or in kibibytes (negative)
        defaultDescription: -2000
    journal_size_limit:
        type: integer
        description: limit the size of rollback-journal and WAL files
        defaultDescription: 67108864
    mmap_size:
        type: integer
        description: max number of bytes that are set aside for memory-mapped I/O
        defaultDescription: 134217728
    page_size:
        type: integer
        description: page size of the database
        defaultDescription: 4096
    persistent-prepared-statements:
        type: boolean
        description: cache prepared statements or not
        defaultDescription: true
    max_prepared_cache_size:
        type: integer
        description: prepared statements cache size limit
        defaultDescription: 200
    initial_read_only_pool_size:
        type: integer
        description: number of read only connections created initially
        defaultDescription: 5
    max_read_only_pool_size:
        type: integer
        description: maximum number of created read only connections
        defaultDescription: 10
)");
}

}  // namespace components

USERVER_NAMESPACE_END
