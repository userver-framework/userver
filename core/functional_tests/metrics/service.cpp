#include <userver/utest/using_namespace_userver.hpp>

#include <userver/cache/caching_component_base.hpp>
#include <userver/components/common_component_list.hpp>
#include <userver/components/common_server_component_list.hpp>
#include <userver/dump/common.hpp>
#include <userver/dump/common_containers.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/utils/daemon_run.hpp>

namespace functional_tests {

using KeyTranslation = std::unordered_map<std::string, std::string>;
static_assert(dump::CheckDumpable<KeyTranslation>());

class HttpCachedTranslations final
    : public components::CachingComponentBase<KeyTranslation> {
 public:
  static constexpr std::string_view kName = "sample-cache";

  HttpCachedTranslations(const components::ComponentConfig& config,
                         const components::ComponentContext& context)
      : CachingComponentBase(config, context) {
    CacheUpdateTrait::StartPeriodicUpdates();
  }

  ~HttpCachedTranslations() override {
    CacheUpdateTrait::StopPeriodicUpdates();
  }

  void Update(cache::UpdateType /*type*/,
              const std::chrono::system_clock::time_point& /*last_update*/,
              const std::chrono::system_clock::time_point& /*now*/,
              cache::UpdateStatisticsScope& stats_scope) override {
    stats_scope.IncreaseDocumentsReadCount(1);
    const auto size = 1;
    Set(KeyTranslation{{"1", "1"}});
    stats_scope.Finish(size);
  }
};

}  // namespace functional_tests

int main(int argc, const char* const argv[]) {
  const auto component_list =
      components::ComponentList()
          .AppendComponentList(components::CommonComponentList())
          .AppendComponentList(components::CommonServerComponentList())
          .Append<components::Secdist>()
          .Append<functional_tests::HttpCachedTranslations>()
          .Append<server::handlers::Ping>();
  return utils::DaemonMain(argc, argv, component_list);
}
