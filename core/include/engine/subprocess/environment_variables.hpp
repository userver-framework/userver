#pragma once

#include <string>
#include <unordered_map>

namespace engine {
namespace subprocess {

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

const EnvironmentVariables& GetCurrentEnvironmentVariables();

}  // namespace subprocess
}  // namespace engine
