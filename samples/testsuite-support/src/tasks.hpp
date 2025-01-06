#pragma once

#include <userver/components/component_base.hpp>
#include <userver/utest/using_namespace_userver.hpp>

namespace tests::handlers {

class TasksSample final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "tasks-sample";

    TasksSample(const components::ComponentConfig&, const components::ComponentContext&);
};

}  // namespace tests::handlers
