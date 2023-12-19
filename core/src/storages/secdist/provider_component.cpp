#include <fstream>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/engine/subprocess/environment_variables.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/yaml/serialize.hpp>
#include <userver/storages/secdist/exceptions.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/utils/async.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::parse {

static formats::json::Value Parse(const formats::yaml::Value& yaml,
                                  formats::parse::To<formats::json::Value>) {
  formats::json::ValueBuilder json_vb;

  if (yaml.IsBool()) {
    json_vb = yaml.As<bool>();
  } else if (yaml.IsInt64()) {
    json_vb = yaml.As<int64_t>();
  } else if (yaml.IsUInt64()) {
    json_vb = yaml.As<uint64_t>();
  } else if (yaml.IsDouble()) {
    json_vb = yaml.As<double>();
  } else if (yaml.IsString()) {
    json_vb = yaml.As<std::string>();
  } else if (yaml.IsArray()) {
    json_vb = {formats::common::Type::kArray};
    for (const auto& elem : yaml) {
      json_vb.PushBack(elem.As<formats::json::Value>());
    }
  } else if (yaml.IsObject()) {
    json_vb = {formats::common::Type::kObject};
    for (auto it = yaml.begin(); it != yaml.end(); ++it) {
      json_vb[it.GetName()] = it->As<formats::json::Value>();
    }
  }
  return json_vb.ExtractValue();
}

}  // namespace formats::parse

namespace storages::secdist {

namespace {

formats::json::Value DoLoadFromFile(const std::string& path,
                                    SecdistFormat format, bool missing_ok) {
  formats::json::Value doc;
  if (path.empty()) return doc;

  std::ifstream stream(path);
  try {
    if (format == SecdistFormat::kJson) {
      doc = formats::json::FromStream(stream);
    } else if (format == SecdistFormat::kYaml) {
      const auto yaml_doc = formats::yaml::FromStream(stream);
      doc = yaml_doc.As<formats::json::Value>();
    }
  } catch (const std::exception& e) {
    if (missing_ok) {
      LOG_WARNING() << "Failed to load secdist from file '" << path
                    << "': " << e << ", booting without secdist";
    } else {
      throw SecdistError(
          "Cannot load secdist config. File '" + path +
          "' doesn't exist, unreachable or in invalid format:" + e.what());
    }
  }

  return doc;
}

formats::json::Value LoadFromFile(
    const std::string& path, SecdistFormat format, bool missing_ok,
    engine::TaskProcessor* blocking_task_processor) {
  if (blocking_task_processor)
    return utils::Async(*blocking_task_processor, "load_secdist_from_file",
                        &DoLoadFromFile, std::cref(path), format, missing_ok)
        .Get();
  else
    return DoLoadFromFile(path, format, missing_ok);
}

void MergeJsonObj(formats::json::ValueBuilder& builder,
                  const formats::json::Value& update) {
  if (!update.IsObject()) {
    builder = update;
    return;
  }

  for (auto it = update.begin(); it != update.end(); ++it) {
    auto sub_node = builder[it.GetName()];
    MergeJsonObj(sub_node, *it);
  }
}

void UpdateFromEnv(formats::json::Value& doc,
                   const std::optional<std::string>& environment_secrets_key) {
  if (!environment_secrets_key) return;

  const auto& env_vars =
      engine::subprocess::GetCurrentEnvironmentVariablesPtr();
  const auto* value = env_vars->GetValueOptional(*environment_secrets_key);
  if (value) {
    formats::json::Value value_json;
    try {
      value_json = formats::json::FromString(*value);
    } catch (const std::exception& ex) {
      throw SecdistError("Can't parse '" + *environment_secrets_key +
                         "' env variable: " + ex.what());
    }
    formats::json::ValueBuilder doc_builder(doc);
    MergeJsonObj(doc_builder, value_json);
    doc = doc_builder.ExtractValue();
  }
}

}  // namespace

DefaultLoader::DefaultLoader(Settings settings) : settings_{settings} {}

formats::json::Value DefaultLoader::Get() const {
  auto doc =
      LoadFromFile(settings_.config_path, settings_.format,
                   settings_.missing_ok, settings_.blocking_task_processor);
  UpdateFromEnv(doc, settings_.environment_secrets_key);
  return doc;
}

}  // namespace storages::secdist

namespace components {

namespace {

storages::secdist::SecdistFormat FormatFromString(std::string_view str) {
  if (str.empty() || str == "json") {
    return storages::secdist::SecdistFormat::kJson;
  } else if (str == "yaml") {
    return storages::secdist::SecdistFormat::kYaml;
  }

  UINVARIANT(
      false,
      fmt::format("Unknown secdist format '{}' (must be one of 'json', 'yaml')",
                  str));
}

storages::secdist::DefaultLoader::Settings ParseSettings(
    const components::ComponentConfig& config,
    const components::ComponentContext& context) {
  storages::secdist::DefaultLoader::Settings settings;
  auto blocking_task_processor_name =
      config["blocking-task-processor"].As<std::optional<std::string>>();
  settings.blocking_task_processor =
      blocking_task_processor_name
          ? &context.GetTaskProcessor(*blocking_task_processor_name)
          : nullptr;
  settings.config_path = config["config"].As<std::string>({});
  settings.format = FormatFromString(config["format"].As<std::string>({}));
  settings.missing_ok = config["missing-ok"].As<bool>(false);
  settings.environment_secrets_key =
      config["environment-secrets-key"].As<std::optional<std::string>>();

  return settings;
}

}  // namespace

DefaultSecdistProvider::DefaultSecdistProvider(const ComponentConfig& config,
                                               const ComponentContext& context)
    : LoggableComponentBase{config, context},
      loader_{ParseSettings(config, context)} {}

formats::json::Value DefaultSecdistProvider::Get() const {
  return loader_.Get();
}

yaml_config::Schema DefaultSecdistProvider::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<LoggableComponentBase>(R"(
type: object
description: Component that stores security related data (keys, passwords, ...).
additionalProperties: false
properties:
    config:
        type: string
        description: path to the config file with data
        defaultDescription: ''
    format:
        type: string
        description: secdist format
        defaultDescription: 'json'
    missing-ok:
        type: boolean
        description: do not terminate components load if no file found by the config option
        defaultDescription: false
    environment-secrets-key:
        type: string
        description: name of environment variable from which to load additional data
    blocking-task-processor:
        type: string
        description: name of task processor for background blocking operations
)");
}

}  // namespace components

USERVER_NAMESPACE_END
