#pragma once

#ifdef USERVER_FEATURE_LEGACY_REDIS_NAMESPACE

#include <userver/storages/redis/base.hpp>

USERVER_NAMESPACE_BEGIN
namespace redis {
using storages::redis::CommandsBufferingSettings;
using storages::redis::ConnectionInfo;
using storages::redis::ConnectionSecurity;
using storages::redis::Password;
using storages::redis::PublishSettings;
using storages::redis::PubsubMetricsSettings;
using storages::redis::ReplicationMonitoringSettings;
using storages::redis::ScanCursor;
using storages::redis::Stat;
}  // namespace redis

USERVER_NAMESPACE_END

#else

#error This header is deprecated, use <userver/storages/redis/base.hpp> instead

#endif
