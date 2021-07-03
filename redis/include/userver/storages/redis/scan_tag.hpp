#pragma once

namespace storages {
namespace redis {

enum class ScanTag { kScan, kSscan, kHscan, kZscan };

}  // namespace redis
}  // namespace storages
