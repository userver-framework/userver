#include <userver/storages/secdist/component.hpp>

#include <userver/components/component.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/secdist/exceptions.hpp>
#include <userver/utils/string_to_duration.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

namespace {

storages::secdist::SecdistConfig::Settings ParseSettings(
    const ComponentConfig& config, const ComponentContext& context) {
  storages::secdist::SecdistConfig::Settings settings;
  settings.config_path = config["config"].As<std::string>({});
  settings.missing_ok = config["missing-ok"].As<bool>(false);
  settings.environment_secrets_key =
      config["environment-secrets-key"].As<std::optional<std::string>>();
  settings.update_period =
      utils::StringToDuration(config["update-period"].As<std::string>("0s"));
  auto blocking_task_processor_name =
      config["blocking-task-processor"].As<std::optional<std::string>>();
  settings.blocking_task_processor =
      blocking_task_processor_name
          ? &context.GetTaskProcessor(*blocking_task_processor_name)
          : nullptr;
  return settings;
}

}  // namespace

Secdist::Secdist(const ComponentConfig& config, const ComponentContext& context)
    : LoggableComponentBase(config, context),
      secdist_(ParseSettings(config, context)) {}

const storages::secdist::SecdistConfig& Secdist::Get() const {
  return secdist_.Get();
}

rcu::ReadablePtr<storages::secdist::SecdistConfig> Secdist::GetSnapshot()
    const {
  return secdist_.GetSnapshot();
}

storages::secdist::Secdist& Secdist::GetStorage() { return secdist_; }

}  // namespace components

USERVER_NAMESPACE_END
