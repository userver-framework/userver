#pragma once

#include <optional>
#include <stdexcept>
#include <utility>

#include <userver/cache/cache_update_trait.hpp>
#include <userver/fs/blocking/temp_directory.hpp>
#include <userver/testsuite/cache_control.hpp>
#include <userver/testsuite/dump_control.hpp>
#include <userver/yaml_config/yaml_config.hpp>
#include <utest/utest.hpp>

// Note: the associated cpp file is "internal_helpers_test.cpp"

namespace cache {

class CacheMockBase : public CacheUpdateTrait {
 protected:
  CacheMockBase(std::string_view name, const yaml_config::YamlConfig& config,
                testsuite::CacheControl& cache_control);
};

class DumpableCacheMockBase : public CacheUpdateTrait {
 protected:
  DumpableCacheMockBase(std::string_view name,
                        const yaml_config::YamlConfig& config,
                        const fs::blocking::TempDirectory& dump_root,
                        testsuite::CacheControl& cache_control,
                        testsuite::DumpControl& dump_control);

 private:
  DumpableCacheMockBase(std::string_view name,
                        const yaml_config::YamlConfig& config,
                        const dump::Config& dump_config,
                        testsuite::CacheControl& cache_control,
                        testsuite::DumpControl& dump_control);
};

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

}  // namespace cache
