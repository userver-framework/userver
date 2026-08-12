#include <userver/hostinfo/cpu_limit.hpp>

#include <cmath>
#include <optional>
#include <string>
#include <thread>

#include <userver/fs/blocking/read.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/from_string.hpp>
#include <userver/utils/text_light.hpp>

USERVER_NAMESPACE_BEGIN

namespace hostinfo {

namespace {

constexpr utils::zstring_view kProcSelfCgroup = "/proc/self/cgroup";
constexpr std::string_view kSysFsCgroup = "/sys/fs/cgroup";

std::optional<double> CpuLimitDeploy() {
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    const char* cpu_limit_c_str = std::getenv("DEPLOY_VCPU_LIMIT");
    if (!cpu_limit_c_str) {
        LOG_INFO() << "DEPLOY_VCPU_LIMIT env is unset, ignoring it";
        return {};
    }

    const std::string_view cpu_limit{cpu_limit_c_str};
    LOG_DEBUG() << "DEPLOY_VCPU_LIMIT='" << cpu_limit << "'";

    try {
        // Note: the division is precise for reasonably small integer values.
        return utils::FromString<double>(cpu_limit) / 1000;
    } catch (const std::exception& e) {
        LOG_ERROR() << "Failed to parse DEPLOY_VCPU_LIMIT: " << e;
        return {};
    }
}

std::optional<double> CpuLimitRtc() {
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    const char* cpu_limit_c_str = std::getenv("CPU_LIMIT");
    if (!cpu_limit_c_str) {
        LOG_INFO() << "CPU_LIMIT env is unset, ignoring it";
        return {};
    }

    const std::string_view cpu_limit{cpu_limit_c_str};
    LOG_DEBUG() << "CPU_LIMIT='" << cpu_limit << "'";

    if (!cpu_limit.empty() && cpu_limit.back() == 'c') {
        try {
            return utils::FromString<double>(cpu_limit.substr(0, cpu_limit.size() - 1));
        } catch (const std::exception& e) {
            LOG_ERROR() << "Failed to parse CPU_LIMIT: " << e;
        }
    }
    LOG_ERROR() << "CPU_LIMIT env is invalid (" << cpu_limit << "), ignoring it";

    return {};
}

std::optional<std::string> GetSelfCgroup() {
    std::string contents;
    try {
        contents = fs::blocking::ReadFileContents(kProcSelfCgroup);
    } catch (const std::exception& e) {
        LOG_WARNING() << "Failed to read " << kProcSelfCgroup << ": " << e;
        return {};
    }

    for (const auto& line : utils::text::SplitIntoStringViewVector(contents, "\n")) {
        // Format: hierarchy-id:controllers:path
        const auto parts = utils::text::SplitIntoStringViewVector(line, ":");
        if (parts.size() < 3) {
            continue;
        }
        if (parts[0] != "0") {
            // Cgroup v2 has hierarchy_i == 0
            continue;
        }

        std::string path{parts[2]};
        for (std::size_t i = 3; i < parts.size(); ++i) {
            path += ':';
            path += parts[i];
        }
        return path;
    }
    return {};
}

std::optional<int> CpuLimitCgroupV2() {
    auto cgroup = GetSelfCgroup();
    if (!cgroup) {
        return {};
    }

    auto cpu_max_path = fmt::format("{}/{}/cpu.max", kSysFsCgroup, *cgroup);

    std::string content;
    try {
        content = fs::blocking::ReadFileContents(cpu_max_path);
    } catch (const std::exception& e) {
        LOG_WARNING() << "Failed to read " << cpu_max_path << ": " << e;
        return {};
    }

    const auto parts = utils::text::SplitIntoStringViewVector(utils::text::TrimView(content), " \t");
    if (parts.size() != 2) {
        return {};
    }

    if (parts[0] == "max") {
        return {};
    }

    const auto cpu_max = utils::FromString<int>(parts[0]);
    const auto cpu_period = utils::FromString<int>(parts[1]);
    if (cpu_period == 0) {
        LOG_WARNING() << "cpu.max period is zero";
        return {};
    }

    return static_cast<double>(cpu_max) / static_cast<double>(cpu_period);
}

double GetCpuLimit() {
    auto limit = CpuLimitRtc();
    if (limit) {
        return *limit;
    }

    limit = CpuLimitDeploy();
    if (limit) {
        return *limit;
    }

    limit = CpuLimitCgroupV2();
    if (limit) {
        return *limit;
    }
    return 0;
}

}  // namespace

std::optional<double> CpuLimit() {
    static const auto kLimit = GetCpuLimit();
    return kLimit;
}

bool IsInRtc() { return !!CpuLimit(); }

}  // namespace hostinfo

USERVER_NAMESPACE_END
