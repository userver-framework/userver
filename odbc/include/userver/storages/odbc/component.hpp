#pragma once

#include <memory>

#include <userver/components/component_base.hpp>
#include <userver/utils/statistics/entry.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {
class Cluster;
}

namespace components {

class Odbc final : public ComponentBase {
public:
    static constexpr std::string_view kName = "odbc";

    Odbc(const ComponentConfig& config, const ComponentContext& context);
    ~Odbc() override;

    std::shared_ptr<storages::odbc::Cluster> GetCluster() const;

    static yaml_config::Schema GetStaticConfigSchema();

private:
    std::shared_ptr<storages::odbc::Cluster> cluster_;
    utils::statistics::Entry statistics_holder_;
};

}  // namespace components

USERVER_NAMESPACE_END
