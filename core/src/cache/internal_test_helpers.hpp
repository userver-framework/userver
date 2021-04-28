#pragma once

#include <optional>
#include <stdexcept>
#include <utility>

#include <cache/cache_update_trait.hpp>
#include <components/component_config.hpp>
#include <dump/internal_test_helpers.hpp>
#include <testsuite/cache_control.hpp>
#include <testsuite/dump_control.hpp>
#include <utest/utest.hpp>

// Note: the associated cpp file is "internal_helpers_test.cpp"

namespace cache {

class CacheMockBase : public CacheUpdateTrait {
 protected:
  CacheMockBase(const components::ComponentConfig& config,
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

using dump::ConfigFromYaml;

}  // namespace cache
