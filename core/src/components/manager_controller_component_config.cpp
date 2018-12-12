#include <components/manager_controller_component_config.hpp>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/types.hpp>
#include <storages/mongo/mongo.hpp>

namespace {

engine::TaskProcessorSettings ParseTaskProcessorSettings(
    const storages::mongo::DocumentElement& elem) {
  engine::TaskProcessorSettings settings;

  namespace sm = storages::mongo;
  const auto& doc = sm::ToDocument(elem);
  const bsoncxx::document::view& overload_doc =
      doc["wait_queue_overload"].get_document();

  settings.wait_queue_time_limit =
      std::chrono::microseconds(sm::ToInt64(overload_doc["time_limit_us"]));
  settings.wait_queue_length_limit = sm::ToInt64(overload_doc["length_limit"]);
  const auto action = sm::ToString(overload_doc["action"]);

  using oa = engine::TaskProcessorSettings::OverloadAction;
  if (action == "cancel")
    settings.overload_action = oa::kCancel;
  else if (action == "ignore")
    settings.overload_action = oa::kIgnore;
  else {
    LOG_ERROR() << "Unknown wait_queue_overload_action: " << action;
  }

  return settings;
}

}  // namespace

namespace components {

// TODO: parse both "default-service" and current service

ManagerControllerTaxiConfig::ManagerControllerTaxiConfig(
    const taxi_config::DocsMap& docs_map)
    : doc(storages::mongo::ToDocument(
          docs_map.Get("USERVER_TASK_PROCESSOR_QOS"))) {
  namespace sm = storages::mongo;
  const bsoncxx::document::view& default_service =
      doc["default-service"].get_document();

  for (const auto& item : default_service)
    settings[std::string(item.key())] = ParseTaskProcessorSettings(item);
  default_settings = settings.at("default-task-processor");
}

}  // namespace components
