#include "session_impl.hpp"
#include <userver/dynamic_config/source.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla::impl {

SessionImpl::SessionImpl(std::string&& id, const SessionConfig session_config, dynamic_config::Source config_source)
    : id_(std::move(id)),
      config_source_(config_source)
{};

const std::string& SessionImpl::Id() const { return id_; }

dynamic_config::Snapshot SessionImpl::GetConfig() const { return config_source_.GetSnapshot(); }

const stats::ScyllaSessionStatistics& SessionImpl::GetStatistics() const { return statistics_; }

stats::ScyllaSessionStatistics& SessionImpl::GetStatistics() { return statistics_; }

}  // namespace storages::scylla::impl

USERVER_NAMESPACE_END
