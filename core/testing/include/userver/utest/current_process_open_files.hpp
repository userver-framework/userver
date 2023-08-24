#pragma once

/// @file userver/utest/current_process_open_files.hpp
/// @brief @copyprief std::vector<std::string> CurrentProcessOpenFiles()

#include <string>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace utest {

/// @brief returns files opened by current process
///
/// jemalloc opens /proc/sys/vm/overcommit_memory, other libraries may also
/// open some files randomly. To avoid problems in tests and make them reliable
/// check files for specific prefix.
std::vector<std::string> CurrentProcessOpenFiles();

}  // namespace utest

USERVER_NAMESPACE_END
