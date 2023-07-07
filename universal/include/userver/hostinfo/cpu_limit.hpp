#pragma once

/// @file userver/hostinfo/cpu_limit.hpp
/// @brief Information about CPU limits in container.

#include <optional>

USERVER_NAMESPACE_BEGIN

/// @brief Information about current host.
namespace hostinfo {

/// @brief Returns the CPU limit.
///
/// Unlike `nproc` and
/// `std::thread::hardware_concurrency` this method considers container limits
/// and may return fractional values.
///
/// Uses:
///   * CPU_LIMIT environment variable (example: `CPU_LIMIT=1.95c`).
std::optional<double> CpuLimit();

/// @brief Returns true if the current process is run in container (CPU_LIMIT
/// environment variable is set).
bool IsInRtc();

}  // namespace hostinfo

USERVER_NAMESPACE_END
