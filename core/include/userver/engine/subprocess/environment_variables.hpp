#pragma once

#include <string>
#include <unordered_map>

#include <userver/rcu/rcu.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::subprocess {

class EnvironmentVariablesUpdate {
 public:
  using Map = std::unordered_map<std::string, std::string>;

  explicit EnvironmentVariablesUpdate(Map vars) : vars_(std::move(vars)) {}

  auto begin() { return vars_.begin(); }
  auto end() { return vars_.end(); }
  auto begin() const { return vars_.begin(); }
  auto end() const { return vars_.end(); }

 private:
  Map vars_;
};

class EnvironmentVariables {
 public:
  using Map = EnvironmentVariablesUpdate::Map;

  explicit EnvironmentVariables(Map vars);
  EnvironmentVariables(const EnvironmentVariables&) = default;

  EnvironmentVariables& UpdateWith(EnvironmentVariablesUpdate update);

  const std::string& GetValue(const std::string& variable_name) const;
  const std::string* GetValueOptional(const std::string& variable_name) const;

  void SetValue(std::string variable_name, std::string value);

  std::string& operator[](const std::string& variable_name);

  auto empty() const { return vars_.empty(); }
  auto size() const { return vars_.size(); }

  using const_iterator = Map::const_iterator;

  auto begin() const { return vars_.begin(); }
  auto end() const { return vars_.end(); }

 private:
  Map vars_;
};

EnvironmentVariables GetCurrentEnvironmentVariables();

rcu::ReadablePtr<EnvironmentVariables> GetCurrentEnvironmentVariablesPtr();

// For using in tests with `setenv()`
void UpdateCurrentEnvironmentVariables();

enum class Overwrite { kAllowed, kForbidden, kIgnored };

void SetEnvironmentVariable(const std::string& variable_name,
                            const std::string& value,
                            Overwrite overwrite = Overwrite::kForbidden);

void UnsetEnvironmentVariable(const std::string& variable_name);

}  // namespace engine::subprocess

USERVER_NAMESPACE_END
