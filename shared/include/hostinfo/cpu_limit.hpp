#pragma once

#include <optional>

namespace hostinfo {

// Returns CPU limit. Unlike `nproc` and
// `std::thread::hardware_concurrency` this method considers container limits
// and may return fractional values.
//
// Uses:
//   * CPU_LIMIT environment variable (example: `CPU_LIMIT=1.95c`).
std::optional<double> CpuLimit();

bool IsInRtc();

}  // namespace hostinfo
