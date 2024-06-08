#include <userver/ydb/dist_lock/settings.hpp>

#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::parse {

ydb::DistLockSettings Parse(const yaml_config::YamlConfig& config,
                            To<ydb::DistLockSettings>) {
  ydb::DistLockSettings settings;
  settings.session_timeout =
      config["session-timeout"].As<std::chrono::milliseconds>(
          settings.session_timeout);
  settings.restart_session_delay =
      config["restart-session-delay"].As<std::chrono::milliseconds>(
          settings.restart_session_delay);
  settings.acquire_interval =
      config["acquire-interval"].As<std::chrono::milliseconds>(
          settings.acquire_interval);
  settings.restart_delay =
      config["restart-delay"].As<std::chrono::milliseconds>(
          settings.restart_delay);
  settings.cancel_task_time_limit =
      config["cancel-task-time-limit"].As<std::chrono::milliseconds>(
          settings.cancel_task_time_limit);
  return settings;
}

}  // namespace formats::parse

USERVER_NAMESPACE_END
