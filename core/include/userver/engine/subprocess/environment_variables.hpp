#pragma once

/// @file userver/engine/subprocess/environment_variables.hpp
/// @brief @copybrief engine::subprocess::EnvironmentVariables

#include <string>
#include <unordered_map>

#include <userver/rcu/rcu.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::subprocess {

/// Iterable evironment variables wrapper to update values.
class EnvironmentVariablesUpdate {
 public:
  using Map = std::unordered_map<std::string, std::string>;

  /// Constructs a wrapper from a map.
  explicit EnvironmentVariablesUpdate(Map vars) : vars_(std::move(vars)) {}

  /// Returns a non const iterator to the beginning.
  auto begin() { return vars_.begin(); }

  /// Returns a non const iterator to the ending.
  auto end() { return vars_.end(); }

  /// Returns a const iterator to the beginning.
  auto begin() const { return vars_.begin(); }

  /// Returns a const iterator to the ending.
  auto end() const { return vars_.end(); }

 private:
  Map vars_;
};

/// @brief Environment variables representation.
///
/// Wrapper to save environment variables and pass them to ProcessStarter::Exec.
/// Changing an instance does not change current environment
/// variables of the process.
class EnvironmentVariables {
 public:
  using Map = EnvironmentVariablesUpdate::Map;

  /// @brief Constructs an instance from pairs: key, value taken from the map.
  explicit EnvironmentVariables(Map vars);

  /// Constructs copy of the instance.
  EnvironmentVariables(const EnvironmentVariables&) = default;

  /// @brief Updates variable.
  /// @note If variable does not exist then it is added.
  EnvironmentVariables& UpdateWith(EnvironmentVariablesUpdate update);

  /// @brief Returns the value of the variable.
  /// @warning Throws std::runtime_eror if there is no variable.
  const std::string& GetValue(const std::string& variable_name) const;

  /// Returns the pointer to the value of the variable or
  /// nullptr if there is no variable.
  const std::string* GetValueOptional(const std::string& variable_name) const;

  /// Sets the value of the variable.
  void SetValue(std::string variable_name, std::string value);

  /// Returns the reference to the value.
  std::string& operator[](const std::string& variable_name);

  /// Checks whether the container is empty.
  auto empty() const { return vars_.empty(); }

  /// Returns the number of elements.
  auto size() const { return vars_.size(); }

  using const_iterator = Map::const_iterator;

  /// Returns a const iterator to the beginning.
  auto begin() const { return vars_.begin(); }

  /// Returns a const iterator to the ending.
  auto end() const { return vars_.end(); }

 private:
  Map vars_;
};

/// Returns copy of the environment variables of the current process.
EnvironmentVariables GetCurrentEnvironmentVariables();

/// Returns thread-safe read only pointer
/// to the environment variables of the current process.
rcu::ReadablePtr<EnvironmentVariables> GetCurrentEnvironmentVariablesPtr();

/// Fetches current environment variables for getting via
/// GetCurrentEnvironmentVariables or GetCurrentEnvrionmentVariablesPtr.
void UpdateCurrentEnvironmentVariables();

/// Overwrite modes
enum class Overwrite {
  kAllowed,    ///< Overwrites or creates the environment variable
  kForbidden,  ///< Creates new enviroment variable, else throws
               ///< std::runtime_error
  kIgnored     ///< Does not overwrite the environment variable if the variable
               ///< exists
};

/// @brief Sets the environment variable with the specified overwrite type.
/// @warning Not thread-safe.
void SetEnvironmentVariable(const std::string& variable_name,
                            const std::string& value,
                            Overwrite overwrite = Overwrite::kForbidden);

/// @brief Unsets the environment variable.
/// @warning Not thread-safe.
void UnsetEnvironmentVariable(const std::string& variable_name);

}  // namespace engine::subprocess

USERVER_NAMESPACE_END
