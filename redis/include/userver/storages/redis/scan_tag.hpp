#pragma once

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

enum class ScanTag { kScan, kSscan, kHscan, kZscan };

}  // namespace storages::redis

USERVER_NAMESPACE_END
