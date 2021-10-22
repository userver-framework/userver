#pragma once

USERVER_NAMESPACE_BEGIN

namespace storages {
namespace redis {

enum class ScanTag { kScan, kSscan, kHscan, kZscan };

}  // namespace redis
}  // namespace storages

USERVER_NAMESPACE_END
