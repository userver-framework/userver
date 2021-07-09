#include <userver/storages/secdist/secdist.hpp>

#include <cerrno>
#include <fstream>

#include <userver/compiler/demangle.hpp>
#include <userver/engine/subprocess/environment_variables.hpp>
#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/secdist/exceptions.hpp>

namespace storages::secdist {

namespace {

std::vector<std::function<std::any(const formats::json::Value&)>>&
GetConfigFactories() {
  static std::vector<std::function<std::any(const formats::json::Value&)>>
      factories;
  return factories;
}

formats::json::Value LoadFromFile(const std::string& path, bool missing_ok) {
  formats::json::Value doc;
  if (path.empty()) return doc;

  std::ifstream json_stream(path);
  try {
    doc = formats::json::FromStream(json_stream);
  } catch (const std::exception& e) {
    if (missing_ok) {
      LOG_WARNING() << "Failed to load secdist from file: " << e
                    << ", booting without secdist";
    } else {
      throw SecdistError(
          "Cannot load secdist config. File '" + path +
          "' doesn't exist, unrechable or in invalid format:" + e.what());
    }
  }

  return doc;
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

  const auto& env_vars = engine::subprocess::GetCurrentEnvironmentVariables();
  const auto* value = env_vars.GetValueOptional(*environment_secrets_key);
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

SecdistConfig::SecdistConfig() = default;

SecdistConfig::SecdistConfig(
    const std::string& path, bool missing_ok,
    const std::optional<std::string>& environment_secrets_key) {
  // if we don't want to read secdist, then we don't need to initialize
  if (GetConfigFactories().empty()) return;

  auto doc = LoadFromFile(path, missing_ok);
  UpdateFromEnv(doc, environment_secrets_key);

  Init(doc);
}

void SecdistConfig::Init(const formats::json::Value& doc) {
  for (const auto& config_factory : GetConfigFactories()) {
    configs_.emplace_back(config_factory(doc));
  }
}

std::size_t SecdistConfig::Register(
    std::function<std::any(const formats::json::Value&)>&& factory) {
  auto& config_factories = GetConfigFactories();
  config_factories.emplace_back(std::move(factory));
  return config_factories.size() - 1;
}

const std::any& SecdistConfig::Get(const std::type_index& type,
                                   std::size_t index) const {
  try {
    return configs_.at(index);
  } catch (const std::out_of_range&) {
    throw std::out_of_range("Type " + compiler::GetTypeName(type) +
                            " is not registered as config");
  }
}

}  // namespace storages::secdist
