#include <userver/dynamic_config/impl/snapshot.hpp>

#include <iterator>
#include <stdexcept>
#include <utility>

#include <fmt/format.h>

#include <userver/compiler/demangle.hpp>
#include <userver/dynamic_config/exception.hpp>
#include <userver/dynamic_config/storage_mock.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/cpu_relax.hpp>
#include <userver/utils/enumerate.hpp>
#include <userver/utils/impl/static_registration.hpp>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config::impl {
namespace {

struct VariableMetadata final {
    std::string name;
    Factory factory;
};

std::vector<VariableMetadata>& Registry() {
    static std::vector<VariableMetadata> registry;
    return registry;
}

std::vector<ConfigMetadata>& ConfigMetadataRegistry() {
    static std::vector<ConfigMetadata> registry;
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

ConfigId Register(VariableMetadata&& metadata) {
    utils::impl::AssertStaticRegistrationAllowed("dynamic_config::Key registration");
    auto& registry = Registry();
    registry.push_back(std::move(metadata));
    return registry.size() - 1;
}

}  // namespace

[[noreturn]] void WrapGetError(const std::exception& ex, std::type_index type) {
    throw std::logic_error(fmt::format("Error in Config::Get<{}>: {}", compiler::GetTypeName(type), ex.what()));
}

formats::json::Value DocsMapGet(const DocsMap& docs_map, std::string_view key) { return docs_map.Get(key); }

ConfigId RegisterInternal(std::string&& name, Factory factory) {
    return Register(VariableMetadata{
        .name = std::move(name),
        .factory = std::move(factory),
    });
}

ConfigId Register(Factory factory, std::vector<ConfigMetadata>&& config_metadata) {
    for (const auto& config : config_metadata) {
        UASSERT_MSG(
            IsValidJson(config.default_as_json_string),
            fmt::format("Invalid default JSON for dynamic config '{}': {}", config.name, config.default_as_json_string)
        );
    }
    const auto id = Register(VariableMetadata{
        .name = config_metadata.size() == 1 ? config_metadata.front().name : std::string{},
        .factory = std::move(factory),
    });
    auto& registry = ConfigMetadataRegistry();
    registry.insert(
        registry.end(),
        std::make_move_iterator(config_metadata.begin()),
        std::make_move_iterator(config_metadata.end())
    );
    return id;
}

std::vector<dynamic_config::RegisteredConfigMeta> GetRegisteredConfigsMeta() {
    utils::impl::AssertStaticRegistrationFinished();
    const auto& registry = ConfigMetadataRegistry();
    std::vector<dynamic_config::RegisteredConfigMeta> result;
    result.reserve(registry.size());
    for (const auto& meta : registry) {
        result.push_back(dynamic_config::RegisteredConfigMeta{
            .name = meta.name,
            .schema_hash = meta.schema_hash,
            .default_as_json_string = meta.default_as_json_string,
        });
    }
    return result;
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

    for (const auto& config : ConfigMetadataRegistry()) {
        formats::json::Value default_value;
        try {
            default_value = formats::json::FromString(config.default_as_json_string);
        } catch (const std::exception& ex) {
            throw std::runtime_error(fmt::format(
                "Invalid in-code default JSON value '{}' for dynamic "
                "config variable '{}': {}",
                config.default_as_json_string,
                config.name,
                ex.what()
            ));
        }

        if (result.Has(config.name) && result.Get(config.name) != default_value) {
            throw std::runtime_error(fmt::format(
                "Default value for dynamic config variable '{}' is specified "
                "multiple times, and those values differ: '{}' != '{}'",
                config.name,
                ToString(result.Get(config.name)),
                ToString(default_value)
            ));
        }
        // Keep the config name in JSON paths reported by config parsers.
        formats::json::ValueBuilder named_default{formats::json::Type::kObject};
        named_default.EmplaceNocheck(config.name, std::move(default_value));
        result.Set(config.name, named_default.ExtractValue()[config.name]);
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

SnapshotData::SnapshotData(const DocsMap& defaults, const std::vector<KeyValue>& overrides)
    : SnapshotData(overrides)
{
    utils::StreamingCpuRelax relax(1, nullptr);
    for (const auto [id, metadata] : utils::enumerate(Registry())) {
        if (!user_configs_[id].has_value()) {
            relax.Relax(1);
            try {
                user_configs_[id] = metadata.factory(defaults);
            } catch (const std::exception& ex) {
                const auto
                    name = metadata.name.empty() ? "with custom DocsMap parser" : std::string_view{metadata.name};
                throw ConfigParseError(fmt::format(
                    "{} while parsing dynamic config {}. {}",
                    compiler::GetTypeName(typeid(ex)),
                    name,
                    ex.what()
                ));
            }
        }
    }
}

SnapshotData::SnapshotData(const SnapshotData& defaults, const std::vector<KeyValue>& overrides)
    : SnapshotData(overrides)
{
    if (defaults.IsEmpty()) {
        return;
    }

    for (const auto [id, factory] : utils::enumerate(Registry())) {
        if (user_configs_[id].has_value()) {
            continue;
        }
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
