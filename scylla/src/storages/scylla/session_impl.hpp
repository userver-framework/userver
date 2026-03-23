#pragma once

#include <string>
#include <userver/dynamic_config/source.hpp>
#include <userver/storages/scylla/session_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla::impl {
class SessionImpl {
public:
    SessionImpl(SessionImpl&&) = delete;
    SessionImpl& operator=(SessionImpl&&) = delete;

    const std::string& Id() const;
    dynamic_config::Snapshot GetConfig() const;

    virtual const std::string& DefaultDatabaseName() const = 0;

    // unimplemented
    virtual void Ping() = 0;

    // unimplemented
    virtual void SetConnectionString(const std::string& connection_string) = 0;

protected:
    SessionImpl(std::string&& id, const SessionConfig session_config, dynamic_config::Source config_source);

private:
    const std::string id_;
    const dynamic_config::Source config_source_;
};

using SessionImplPtr = std::shared_ptr<SessionImpl>;

}  // namespace storages::scylla::impl

USERVER_NAMESPACE_END