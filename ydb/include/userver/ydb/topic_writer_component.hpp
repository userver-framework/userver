#pragma once

/// @file userver/ydb/topic_writer_component.hpp
/// @brief @copybrief ydb::TopicWriterComponent

#include <memory>

#include <userver/compiler/impl/lifetime.hpp>
#include <userver/components/component_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb {

class TopicWriterManager;

/// @ingroup userver_components
///
/// @brief userver component for YDB topic writer
///
/// ## Static options of ydb::TopicWriterComponent :
/// @include{doc} scripts/docs/en/components_schema/ydb/src/ydb/topic_writer_component.md
///
/// Options inherited from @ref components::ComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
class TopicWriterComponent final : public components::ComponentBase {
public:
    /// Component name for use in static config
    static constexpr std::string_view kName = "ydb-topic-writer";

    TopicWriterComponent(const components::ComponentConfig&, const components::ComponentContext&);

    ~TopicWriterComponent() final;

    /// @brief Returns the underlying TopicWriterManager instance
    TopicWriterManager& GetTopicWriterManager() USERVER_IMPL_LIFETIME_BOUND;

    static yaml_config::Schema GetStaticConfigSchema();

private:
    std::shared_ptr<TopicWriterManager> writer_manager_;
};

}  // namespace ydb

USERVER_NAMESPACE_END
