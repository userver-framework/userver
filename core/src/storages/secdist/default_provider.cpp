#include <fstream>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/engine/subprocess/environment_variables.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/yaml/serialize.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/secdist/exceptions.hpp>
#include <userver/storages/secdist/default_provider.hpp>
#include <userver/storages/secdist/exceptions.hpp>
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

DefaultProvider::DefaultProvider(Settings settings) : settings_{settings} {}

formats::json::Value DefaultProvider::Get() const {
  auto doc =
      LoadFromFile(settings_.config_path, settings_.format,
                   settings_.missing_ok, settings_.blocking_task_processor);
  UpdateFromEnv(doc, settings_.environment_secrets_key);
  return doc;
}

}  // namespace storages::secdist

USERVER_NAMESPACE_END
