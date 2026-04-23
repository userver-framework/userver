#pragma once

#include <userver/components/component_base.hpp>
#include <userver/storages/secdist/secdist.hpp>
#include <userver/utils/statistics/entry.hpp>

#include <string>

#include "session.hpp"

USERVER_NAMESPACE_BEGIN

namespace components {

class Scylla : public ComponentBase {
public:
    Scylla(const ComponentConfig&, const ComponentContext&);

    ~Scylla() override;

    storages::scylla::SessionPtr GetSession() const;
    static yaml_config::Schema GetStaticConfigSchema();

private:
    void OnSecdistUpdate(const storages::secdist::SecdistConfig& config);

    std::string dbalias_;
    storages::scylla::SessionPtr session_;

    utils::statistics::Entry statistics_entry_;
    concurrent::AsyncEventSubscriberScope secdist_subscriber_;
};

}  // namespace components

USERVER_NAMESPACE_END
