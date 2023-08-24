#pragma once

#include <string>
#include <system_error>

USERVER_NAMESPACE_BEGIN

namespace utils::jemalloc {

std::string Stats();

std::error_code ProfActivate();

std::error_code ProfDeactivate();

std::error_code ProfDump();

std::error_code SetMaxBgThreads(size_t max_bg_threads);

std::error_code EnableBgThreads();

// blocking
std::error_code StopBgThreads();

}  // namespace utils::jemalloc

USERVER_NAMESPACE_END
