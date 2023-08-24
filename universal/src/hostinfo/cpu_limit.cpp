#include <userver/hostinfo/cpu_limit.hpp>

#include <optional>
#include <string>

#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace hostinfo {

namespace {

std::optional<double> CpuLimitRtc() {
  // NOLINTNEXTLINE(concurrency-mt-unsafe)
  const char* cpu_limit_c_str = std::getenv("CPU_LIMIT");
  if (!cpu_limit_c_str) {
    LOG_INFO() << "CPU_LIMIT env is unset, ignoring it";
    return {};
  }

  std::string cpu_limit(cpu_limit_c_str);
  LOG_DEBUG() << "CPU_LIMIT='" << cpu_limit << "'";

  try {
    size_t end{0};
    auto cpu_f = std::stod(cpu_limit, &end);
    if (cpu_limit.substr(end) != "c") {
      return {};
    }

    return {cpu_f};
  } catch (const std::exception& e) {
    LOG_ERROR() << "Failed to parse CPU_LIMIT: " << e;
  }
  LOG_ERROR() << "CPU_LIMIT env is invalid (" << cpu_limit << "), ignoring it";

  return {};
}

}  // namespace

std::optional<double> CpuLimit() {
  static const auto limit = CpuLimitRtc();
  return limit;
}

bool IsInRtc() { return !!CpuLimitRtc(); }

}  // namespace hostinfo

USERVER_NAMESPACE_END
