#include <userver/storages/rocks/component.hpp>

#include <optional>

#include <userver/storages/rocks/db.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

yaml_config::Schema Rocks::GetStaticConfigSchema() {
    return yaml_config::MergeSchemas<ComponentBase>(R"(
type: object
description: RocksDB component
additionalProperties: false
properties:
    db_path:
        type: string
        description: path to the database file
    max_background_jobs:
        type: integer
        minimum: 1
        description: maximum number of concurrent background jobs, including flushes and compactions
    column_families:
        type: array
        items:
            type: string
            description: name of column family
        description: list of initial column families
    compression:
        type: string
        description: compress blocks using this compression algorithm
    compression_level:
        type: integer
        description: compression level applicable to zstd and lz4
    bottommost_compression:
        type: string
        description: compress bottommost blocks using this compression algorithm
    bottommost_compression_level:
        type: integer
        description: compression level applicable to zstd and lz4
    use_direct_reads:
        type: boolean
        description: enable direct I/O mode for read/write
    use_direct_io_for_flush_and_compaction:
        type: boolean
        description: use O_DIRECT for writes in background flush and compactions
)");
}

Rocks::Rocks(const components::ComponentConfig& config, const components::ComponentContext& context)
    : ComponentBase{config, context}, db_ptr_{std::make_shared<storages::rocks::Db>(
        config["db_path"].As<std::string>(),
        config["max_background_jobs"].As<int>(),
        config["column_families"].As<std::vector<std::string>>(),
        storages::rocks::DbOptions{
            config["compression"].As<std::optional<std::string>>(),
            config["compression_level"].As<std::optional<int>>(),
            config["bottommost_compression"].As<std::optional<std::string>>(),
            config["bottommost_compression_level"].As<std::optional<int>>(),
            config["use_direct_reads"].As<std::optional<bool>>(),
            config["use_direct_io_for_flush_and_compaction"].As<std::optional<bool>>()
        },
        context.GetTaskProcessor("rocks-task-processor")
    )} {}

const storages::rocks::DbPtr& Rocks::GetDb() const { return db_ptr_; }

}  // namespace components

USERVER_NAMESPACE_END
