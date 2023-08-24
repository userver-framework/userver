#include <userver/utest/using_namespace_userver.hpp>

#include <userver/cache/caching_component_base.hpp>
#include <userver/components/common_component_list.hpp>
#include <userver/components/common_server_component_list.hpp>
#include <userver/components/component_context.hpp>
#include <userver/dump/common.hpp>
#include <userver/dump/common_containers.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/tracing/manager_component.hpp>
#include <userver/utils/daemon_run.hpp>

namespace functional_tests {

using Data = std::unordered_map<int, int>;

class HandlerCacheState;

class CacheSample final : public components::CachingComponentBase<Data> {
 public:
  friend HandlerCacheState;

  static constexpr std::string_view kName = "sample-cache";

  CacheSample(const components::ComponentConfig& config,
              const components::ComponentContext& context)
      : CachingComponentBase(config, context) {
    CacheUpdateTrait::StartPeriodicUpdates();
  }

  ~CacheSample() override { CacheUpdateTrait::StopPeriodicUpdates(); }

  void Update(cache::UpdateType /*type*/,
              const std::chrono::system_clock::time_point& /*last_update*/,
              const std::chrono::system_clock::time_point& /*now*/,
              cache::UpdateStatisticsScope& stats_scope) override {
    static std::atomic<bool> set_empty{false};

    if (set_empty.exchange(true)) {
      try {
        Set(Data{});
      } catch (const cache::EmptyDataError& e) {
        LOG_INFO() << "Trying to install the empty cache";
        Set(Data{{42, 42}});
      }
    } else {
      Set(Data{{1, 1}});
    }
    stats_scope.Finish(1);
  }
};

class HandlerCacheState final : public server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-cache-state";

  HandlerCacheState(const components::ComponentConfig& config,
                    const components::ComponentContext& context)
      : server::handlers::HttpHandlerBase(config, context),
        cache_(context.FindComponent<CacheSample>()) {}
  std::string HandleRequestThrow(
      const server::http::HttpRequest&,
      server::request::RequestContext&) const override {
    cache_.UpdateSyncDebug(cache::UpdateType::kIncremental);
    cache_.UpdateSyncDebug(cache::UpdateType::kIncremental);

    auto data = cache_.Get();
    if (*data == Data{{42, 42}}) {
      return "Magic numbers";
    }
    return "Not magic numbers";
  };

 private:
  CacheSample& cache_;
};

}  // namespace functional_tests

int main(int argc, const char* const argv[]) {
  const auto component_list =
      components::ComponentList()
          .AppendComponentList(components::CommonComponentList())
          .AppendComponentList(components::CommonServerComponentList())
          .Append<functional_tests::CacheSample>()
          .Append<functional_tests::HandlerCacheState>()
          .Append<server::handlers::Ping>();
  return utils::DaemonMain(argc, argv, component_list);
}
