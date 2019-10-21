#pragma once

#include <string>
#include <system_error>

namespace utils::jemalloc {

std::error_code MallCtlBool(const std::string& name, bool new_value);

std::error_code MallCtlInt(const std::string& name, int new_value);

std::error_code MallCtl(const std::string& name);

namespace cmd {

std::string Stats();

std::error_code ProfActivate();

std::error_code ProfDeactivate();

std::error_code ProfDump();

}  // namespace cmd

}  // namespace utils::jemalloc
