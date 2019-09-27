#include <engine/subprocess/environment_variables.hpp>

#include <unistd.h>

#include <cstring>
#include <stdexcept>

extern char** environ;

namespace engine {
namespace subprocess {
namespace {

EnvironmentVariables::Map GetCurrentEnvironmentVariablesMap() {
  EnvironmentVariables::Map map;

  for (auto ptr = environ; *ptr; ++ptr) {
    auto pos = strchr(*ptr, '=');
    if (pos) map.emplace(std::string(*ptr, pos), std::string(pos + 1));
  }

  return map;
}

}  // namespace

EnvironmentVariables::EnvironmentVariables(Map vars) : vars_(std::move(vars)) {}

EnvironmentVariables& EnvironmentVariables::UpdateWith(
    EnvironmentVariablesUpdate update) {
  for (auto& item : update) {
    vars_[item.first] = std::move(item.second);
  }
  return *this;
}

const std::string& EnvironmentVariables::GetValue(
    const std::string& variable_name) const {
  auto result_opt = GetValueOptional(variable_name);
  if (!result_opt)
    throw std::runtime_error("variable with name=" + variable_name +
                             " was not found");
  return *result_opt;
}

const std::string* EnvironmentVariables::GetValueOptional(
    const std::string& variable_name) const {
  auto it = vars_.find(variable_name);
  if (it == vars_.end()) return nullptr;
  return &it->second;
}

void EnvironmentVariables::SetValue(std::string variable_name,
                                    std::string value) {
  vars_[std::move(variable_name)] = std::move(value);
}

std::string& EnvironmentVariables::operator[](
    const std::string& variable_name) {
  return vars_[variable_name];
}

const EnvironmentVariables& GetCurrentEnvironmentVariables() {
  static const EnvironmentVariables current_env{
      GetCurrentEnvironmentVariablesMap()};
  return current_env;
}

}  // namespace subprocess
}  // namespace engine
