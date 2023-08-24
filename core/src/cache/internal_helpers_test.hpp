#pragma once

#include <optional>
#include <stdexcept>
#include <utility>

#include <userver/cache/cache_update_trait.hpp>
#include <userver/dump/config.hpp>
#include <userver/dynamic_config/storage_mock.hpp>
#include <userver/fs/blocking/temp_directory.hpp>
#include <userver/testsuite/cache_control.hpp>
#include <userver/testsuite/dump_control.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/statistics/storage.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#include <cache/cache_dependencies.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache {

struct MockEnvironment final {
  MockEnvironment() = default;
  explicit MockEnvironment(
      testsuite::CacheControl::PeriodicUpdatesMode update_mode)
      : cache_control(update_mode) {}

  dynamic_config::StorageMock config_storage{{dump::kConfigSet, {}},
                                             {cache::kCacheConfigSet, {}}};
  utils::statistics::Storage statistics_storage;
  fs::blocking::TempDirectory dump_root = fs::blocking::TempDirectory::Create();
  testsuite::CacheControl cache_control{
      testsuite::CacheControl::PeriodicUpdatesMode::kDisabled};
  testsuite::DumpControl dump_control{
      testsuite::DumpControl::PeriodicsMode::kDisabled};
};

class CacheMockBase : public CacheUpdateTrait {
 protected:
  CacheMockBase(std::string_view name, const yaml_config::YamlConfig& config,
                MockEnvironment& environment);

 private:
  void Cleanup() override;
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
    if (!data_) {
      throw MockError();
    }
    return *data_;
  }

  void Set(std::optional<T> data) { data_ = std::move(data); }

  int GetFetchCallsCount() const { return fetch_calls_count_; }

 private:
  std::optional<T> data_;
  mutable int fetch_calls_count_{0};
};

}  // namespace cache

USERVER_NAMESPACE_END
