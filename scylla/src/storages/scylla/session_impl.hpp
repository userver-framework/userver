#pragma once

#include <string>
#include <userver/dynamic_config/source.hpp>
#include <userver/storages/scylla/session_config.hpp>

#include <storages/scylla/stats.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla::impl {
class SessionImpl {
public:
    SessionImpl(SessionImpl&&) = delete;
    SessionImpl& operator=(SessionImpl&&) = delete;

    const std::string& Id() const;
    dynamic_config::Snapshot GetConfig() const;
    const stats::ScyllaSessionStatistics& GetStatistics() const;
    stats::ScyllaSessionStatistics& GetStatistics();

    virtual const std::string& DefaultDatabaseName() const = 0;

    virtual void Ping() = 0;

    virtual void DropKeyspace() = 0;

    virtual void SetConnectionString(const std::string& connection_string) = 0;

protected:
    SessionImpl(std::string&& id, const SessionConfig session_config, dynamic_config::Source config_source);

private:
    const std::string id_;
    const dynamic_config::Source config_source_;
    stats::ScyllaSessionStatistics statistics_;
};

using SessionImplPtr = std::shared_ptr<SessionImpl>;

}  // namespace storages::scylla::impl

USERVER_NAMESPACE_END