#pragma once

#include <set>

#include <components/component_config.hpp>

namespace cache {

class MockError : public std::runtime_error {
 public:
  MockError();
};

template <typename T>
class DataSourceMock final {
 public:
  explicit DataSourceMock(std::optional<T> data) : data_(std::move(data)) {}

  /// @brief Called inside `Update` of a test cache to fetch actual data
  /// @throws MockError if "null" is stored in the `DataSourceMock`
  const T& Fetch() const {
    ++fetch_calls_count_;
    return data_ ? *data_ : throw MockError();
  }

  void Set(std::optional<T> data) { data_ = std::move(data); }

  int GetFetchCallsCount() const { return fetch_calls_count_; }

 private:
  std::optional<T> data_;
  mutable int fetch_calls_count_{0};
};

components::ComponentConfig ConfigFromYaml(const std::string& yaml_string,
                                           const std::string& dump_root,
                                           std::string_view cache_name);

/// Create files, writing their own filenames into them
void CreateDumps(const std::vector<std::string>& filenames,
                 const std::string& directory, std::string_view cache_name);

/// @note Returns filenames, not full paths
std::set<std::string> FilenamesInDirectory(const std::string& directory,
                                           std::string_view cache_name);

}  // namespace cache
