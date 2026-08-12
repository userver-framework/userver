#pragma once

#include <userver/components/component_base.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/utest/using_namespace_userver.hpp>

namespace sample {

class FederatedTopicReaderComponent final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "sample-federated-topic-reader";

    FederatedTopicReaderComponent(
        const components::ComponentConfig& config,
        const components::ComponentContext& context
    );

    static yaml_config::Schema GetStaticConfigSchema();

private:
    engine::TaskWithResult<void> read_task_;
};

}  // namespace sample
