#include <userver/engine/subprocess/environment_variables.hpp>

#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <utility>

extern char** environ;

USERVER_NAMESPACE_BEGIN

namespace engine::subprocess {
namespace {

EnvironmentVariables::Map GetCurrentEnvironmentVariablesMap() {
  EnvironmentVariables::Map map;

  for (auto** ptr = environ; *ptr; ++ptr) {
    auto* pos = strchr(*ptr, '=');
    if (pos) map.emplace(std::string(*ptr, pos), std::string(pos + 1));
  }

  return map;
}

rcu::Variable<EnvironmentVariables>& GetCurrentEnvironmentVariablesStorage() {
  static rcu::Variable<EnvironmentVariables> env{
      GetCurrentEnvironmentVariablesMap()};
  return env;
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
  const auto* result_opt = GetValueOptional(variable_name);
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

EnvironmentVariables GetCurrentEnvironmentVariables() {
  return GetCurrentEnvironmentVariablesStorage().ReadCopy();
}

rcu::ReadablePtr<EnvironmentVariables> GetCurrentEnvironmentVariablesPtr() {
  return GetCurrentEnvironmentVariablesStorage().Read();
}

void UpdateCurrentEnvironmentVariables() {
  GetCurrentEnvironmentVariablesStorage().Assign(
      EnvironmentVariables{GetCurrentEnvironmentVariablesMap()});
}

void SetEnvironmentVariable(const std::string& variable_name,
                            const std::string& value, Overwrite overwrite) {
  const auto read_ptr = GetCurrentEnvironmentVariablesPtr();
  const bool value_exist =
      (read_ptr->GetValueOptional(variable_name) != nullptr);
  if (value_exist) {
    UINVARIANT(overwrite != Overwrite::kForbidden,
               "variable with name= " + variable_name +
                   " was set earlier and prohibits rewriting");
    if (overwrite == Overwrite::kIgnored) {
      return;
    }
  }

  // NOLINTNEXTLINE(concurrency-mt-unsafe)
  ::setenv(variable_name.c_str(), value.c_str(), 1);

  UpdateCurrentEnvironmentVariables();
}

void UnsetEnvironmentVariable(const std::string& variable_name) {
  // NOLINTNEXTLINE(concurrency-mt-unsafe)
  ::unsetenv(variable_name.c_str());

  UpdateCurrentEnvironmentVariables();
}

}  // namespace engine::subprocess

USERVER_NAMESPACE_END
