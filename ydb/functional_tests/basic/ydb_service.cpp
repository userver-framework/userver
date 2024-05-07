#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/dynamic_config/client/component.hpp>
#include <userver/dynamic_config/snapshot.hpp>
#include <userver/dynamic_config/source.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/dynamic_config/updater/component.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/server/component.hpp>
#include <userver/server/handlers/server_monitor.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

#include <userver/ydb/component.hpp>
#include <userver/ydb/dist_lock/component_base.hpp>

#include <views/describe-table/post/view.hpp>
#include <views/select-list/post/view.hpp>
#include <views/select-rows/post/view.hpp>
#include <views/upsert-row/post/view.hpp>

namespace sample {

dynamic_config::Key<std::chrono::milliseconds> kSampleDistLockIntervalMs{
    "SAMPLE_DIST_LOCK_INTERVAL_MS",
    dynamic_config::DefaultAsJsonString{"5000"},
};

class SampleDistLock final : public ydb::DistLockComponentBase {
 public:
  static constexpr std::string_view kName = "sample-dist-lock";

  SampleDistLock(const components::ComponentConfig& config,
                 const components::ComponentContext& context)
      : ydb::DistLockComponentBase(config, context),
        config_source_(
            context.FindComponent<components::DynamicConfig>().GetSource()) {
    Start();
  }

  ~SampleDistLock() override { Stop(); }

 protected:
  void DoWork() override;

  void DoWorkTestsuite() override {
    Foo();
    Bar();
  }

 private:
  void Foo() { TESTPOINT("dist-lock-foo", {}); }

  void Bar() { TESTPOINT("dist-lock-bar", {}); }

  dynamic_config::Source config_source_;
};

/// [DoWork]
void SampleDistLock::DoWork() {
  while (!engine::current_task::ShouldCancel()) {
    // Start a new trace_id.
    auto span = tracing::Span::MakeRootSpan("sample-dist-lock");

    // If Foo() or anything in DoWork() throws an exception,
    // DoWork() will be restarted in `restart-delay` seconds.
    Foo();

    // Check for cancellation after cpu-intensive Foo().
    // You must check for cancellation at least every
    // `cancel-task-time-limit`
    // seconds to have time to notice lock prolongation failure.
    if (engine::current_task::ShouldCancel()) break;

    Bar();

    engine::InterruptibleSleepFor(
        config_source_.GetCopy(kSampleDistLockIntervalMs));
  }
}
/// [DoWork]

}  // namespace sample

int main(int argc, char* argv[]) {
  auto component_list = components::MinimalServerComponentList()
                            .Append<components::TestsuiteSupport>()
                            .Append<server::handlers::TestsControl>()
                            .Append<components::DynamicConfigClient>()
                            .Append<components::DynamicConfigClientUpdater>()
                            .Append<components::HttpClient>()
                            .Append<clients::dns::Component>()
                            .Append<components::DefaultSecdistProvider>()
                            .Append<components::Secdist>()
                            .Append<server::handlers::ServerMonitor>()
                            .Append<sample::SelectRowsHandler>()
                            .Append<sample::DescribeTableHandler>()
                            .Append<sample::SelectListHandler>()
                            .Append<sample::UpsertRowHandler>()
                            .Append<ydb::YdbComponent>()
                            .Append<sample::SampleDistLock>();

  return utils::DaemonMain(argc, argv, component_list);
}
