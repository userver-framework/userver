#include <userver/dynamic_config/impl/snapshot.hpp>

#include <fmt/format.h>

#include <userver/compiler/demangle.hpp>
#include <userver/dynamic_config/exception.hpp>
#include <userver/dynamic_config/storage_mock.hpp>
#include <userver/utils/cpu_relax.hpp>
#include <userver/utils/enumerate.hpp>
#include <userver/utils/impl/static_registration.hpp>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config::impl {
namespace {

struct VariableMetadata final {
  std::string name;
  Factory factory;
  std::string default_docs_map_string;
};

std::vector<VariableMetadata>& Registry() {
  static std::vector<VariableMetadata> registry;
  return registry;
}

bool IsValidJson(std::string_view json_string) {
  try {
    [[maybe_unused]] const auto json = formats::json::FromString(json_string);
    return true;
  } catch (const std::exception& ex) {
    return false;
  }
}

}  // namespace

[[noreturn]] void WrapGetError(const std::exception& ex, std::type_index type) {
  throw std::logic_error(fmt::format("Error in Config::Get<{}>: {}",
                                     compiler::GetTypeName(type), ex.what()));
}

formats::json::Value DocsMapGet(const DocsMap& docs_map, std::string_view key) {
  return docs_map.Get(key);
}

ConfigId Register(std::string&& name, Factory factory,
                  std::string&& default_docs_map_string) {
  utils::impl::AssertStaticRegistrationAllowed(
      "dynamic_config::Key registration");
  UASSERT_MSG(
      IsValidJson(default_docs_map_string),
      fmt::format(
          "Defaults passed to dynamic_config::Key form an invalid JSON: {}",
          default_docs_map_string));
  auto& registry = Registry();
  registry.push_back(VariableMetadata{
      /*name=*/std::move(name),
      /*factory=*/factory,
      /*default_docs_map_string=*/std::move(default_docs_map_string),
  });
  return registry.size() - 1;
}

std::any MakeConfig(ConfigId id, const DocsMap& docs_map) {
  utils::impl::AssertStaticRegistrationFinished();
  return Registry()[id].factory(docs_map);
}

std::string_view GetName(ConfigId id) {
  utils::impl::AssertStaticRegistrationFinished();
  const std::string_view result = Registry()[id].name;
  UINVARIANT(!result.empty(), "No name specified for this config");
  return result;
}

DocsMap MakeDefaultDocsMap() {
  utils::impl::AssertStaticRegistrationFinished();
  DocsMap result;

  for (const auto& variable_metadata : Registry()) {
    DocsMap variable_defaults;
    try {
      variable_defaults.Parse(variable_metadata.default_docs_map_string,
                              /*empty_ok=*/true);
    } catch (const std::exception& ex) {
      throw std::runtime_error(
          fmt::format("Invalid in-code default JSON value '{}' for dynamic "
                      "config variable '{}': {}",
                      variable_metadata.default_docs_map_string,
                      variable_metadata.name, ex.what()));
    }

    for (const auto& name : variable_defaults.GetNames()) {
      if (result.Has(name) && result.Get(name) != variable_defaults.Get(name)) {
        throw std::runtime_error(fmt::format(
            "Default value for dynamic config variable '{}' is specified "
            "multiple times, and those values differ: '{}' != '{}'",
            name, ToString(result.Get(name)),
            ToString(variable_defaults.Get(name))));
      }
    }
    result.MergeOrAssign(std::move(variable_defaults));
  }

  return result;
}

SnapshotData::SnapshotData(const std::vector<KeyValue>& config_variables) {
  utils::impl::AssertStaticRegistrationFinished();
  user_configs_.resize(Registry().size());

  for (const auto& config_variable : config_variables) {
    user_configs_[config_variable.GetId()] = config_variable.GetValue();
  }
}

SnapshotData::SnapshotData(const DocsMap& defaults,
                           const std::vector<KeyValue>& overrides)
    : SnapshotData(overrides) {
  utils::StreamingCpuRelax relax(1, nullptr);
  for (const auto [id, metadata] : utils::enumerate(Registry())) {
    if (!user_configs_[id].has_value()) {
      relax.Relax(1);
      try {
        user_configs_[id] = metadata.factory(defaults);
      } catch (const std::exception& ex) {
        throw ConfigParseError(
            fmt::format("While parsing dynamic config values: {} ({})",
                        ex.what(), compiler::GetTypeName(typeid(ex))));
      }
    }
  }
}

SnapshotData::SnapshotData(const SnapshotData& defaults,
                           const std::vector<KeyValue>& overrides)
    : SnapshotData(overrides) {
  if (defaults.IsEmpty()) return;

  for (const auto [id, factory] : utils::enumerate(Registry())) {
    if (user_configs_[id].has_value()) continue;
    user_configs_[id] = defaults.user_configs_[id];
  }
}

bool SnapshotData::IsEmpty() const noexcept { return user_configs_.empty(); }

const std::any& SnapshotData::DoGet(ConfigId id) const {
  UASSERT_MSG(id < user_configs_.size(), "SnapshotData is in an empty state.");
  const auto& config = user_configs_[id];
  if (!config.has_value()) {
    throw std::logic_error("This type is not registered as config");
  }
  return config;
}

}  // namespace dynamic_config::impl

USERVER_NAMESPACE_END
