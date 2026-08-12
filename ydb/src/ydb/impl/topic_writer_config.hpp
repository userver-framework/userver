#pragma once

#include <unordered_map>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>

#include <userver/ydb/topic_writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

std::unordered_map<std::string, ydb::TopicWriterSettings> MakeTopicWriterSettings(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
);

}  // namespace ydb::impl

USERVER_NAMESPACE_END
