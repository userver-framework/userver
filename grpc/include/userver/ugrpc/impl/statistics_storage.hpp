#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include <userver/concurrent/variable.hpp>
#include <userver/engine/shared_mutex.hpp>
#include <userver/utils/impl/transparent_hash.hpp>
#include <userver/utils/statistics/entry.hpp>
#include <userver/utils/statistics/striped_rate_counter.hpp>

#include <userver/ugrpc/impl/statistics.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

/// Allows to create ServiceStatistics and generic MethodStatistics on the fly.
class StatisticsStorage final {
 public:
  explicit StatisticsStorage(utils::statistics::Storage& statistics_storage,
                             StatisticsDomain domain);

  StatisticsStorage(const StatisticsStorage&) = delete;
  StatisticsStorage& operator=(const StatisticsStorage&) = delete;
  ~StatisticsStorage();

  ServiceStatistics& GetServiceStatistics(
      const StaticServiceMetadata& metadata,
      std::optional<std::string> client_name);

  MethodStatistics& GetGenericStatistics(
      std::string_view call_name, std::optional<std::string_view> client_name);

  std::uint64_t GetStartedRequests() const;

 private:
  // Pointer to service name from its metadata is used as a unique service ID
  using ServiceId = const char*;

  struct ServiceKey {
    ServiceId service_id{};
    std::optional<std::string> client_name;
  };

  struct GenericKey {
    std::string call_name;
    std::optional<std::string> client_name;
  };

  struct GenericKeyView {
    GenericKey Dereference() const;

    std::string_view call_name;
    std::optional<std::string_view> client_name;
  };

  struct ServiceKeyComparer {
    bool operator()(ServiceKey lhs, ServiceKey rhs) const;
  };

  struct ServiceKeyHasher {
    std::size_t operator()(const ServiceKey& key) const noexcept;
  };

  struct GenericKeyComparer {
    using is_transparent [[maybe_unused]] = void;

    bool operator()(const GenericKey& lhs, const GenericKey& rhs) const;
    bool operator()(const GenericKeyView& lhs, const GenericKey& rhs) const;
    bool operator()(const GenericKey& lhs, const GenericKeyView& rhs) const;
  };

  struct GenericKeyHasher {
    using is_transparent [[maybe_unused]] = void;

    std::size_t operator()(const GenericKey& key) const noexcept;
    std::size_t operator()(const GenericKeyView& key) const noexcept;
  };

  void ExtendStatistics(utils::statistics::Writer& writer);

  const StatisticsDomain domain_;
  utils::statistics::StripedRateCounter global_started_;
  concurrent::Variable<std::unordered_map<ServiceKey, ServiceStatistics,
                                          ServiceKeyHasher, ServiceKeyComparer>,
                       engine::SharedMutex>
      service_statistics_map_;
  concurrent::Variable<
      utils::impl::TransparentMap<GenericKey, MethodStatistics,
                                  GenericKeyHasher, GenericKeyComparer>,
      engine::SharedMutex>
      generic_statistics_map_;
  // statistics_holder_ must be the last field.
  utils::statistics::Entry statistics_holder_;
};

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
