#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/component_fwd.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/utils/statistics/storage.hpp>
#include <userver/utils/statistics/writer.hpp>

#include <userver/yaml_config/merge_schemas.hpp>

#include <userver/ydb/topic_writer.hpp>
#include <userver/ydb/topic_writer_component.hpp>

#include <ydb/impl/topic_writer_config.hpp>

// YDB headers leak `ARCADIA_ROOT` macro, so we use __has_include()
#if __has_include("generated/src/ydb/topic_writer_component.yaml.hpp")
#include "generated/src/ydb/topic_writer_component.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace ydb {

TopicWriterComponent::TopicWriterComponent(
    const components::ComponentConfig& component_config,
    const components::ComponentContext& component_context
)
    : components::ComponentBase(component_config, component_context)
{
    auto settings = impl::MakeTopicWriterSettings(component_config, component_context);

    writer_manager_ = std::make_shared<ydb::TopicWriterManager>(std::move(settings));

    utils::statistics::RegisterWriterScope(
        component_context,
        "ydb_topic_writer",
        [this](utils::statistics::Writer& writer) { DumpMetric(writer, *writer_manager_); },
        {}
    );
}

TopicWriterComponent::~TopicWriterComponent() = default;

TopicWriterManager& TopicWriterComponent::GetTopicWriterManager() { return *writer_manager_; }

yaml_config::Schema TopicWriterComponent::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<components::ComponentBase>("src/ydb/topic_writer_component.yaml");
}

}  // namespace ydb

USERVER_NAMESPACE_END
