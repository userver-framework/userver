#pragma once

#include <any>
#include <cstddef>
#include <exception>
#include <functional>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <vector>

#include <userver/dynamic_config/fwd.hpp>
#include <userver/dynamic_config/registered_config_meta.hpp>
#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config::impl {

using Factory = std::function<std::any(const DocsMap&)>;

[[noreturn]] void WrapGetError(const std::exception& ex, std::type_index type);

formats::json::Value DocsMapGet(const DocsMap&, std::string_view key);

using ConfigId = std::size_t;

struct ConfigMetadata final {
    std::string name;
    std::string schema_hash;
    std::string default_as_json_string;
};

/// @brief Registers a dynamic config variable.
/// Zero config items represent a constant; one item also supplies the variable name.
ConfigId Register(Factory factory, std::vector<ConfigMetadata>&& config_metadata);

/// @brief Registers an internal derived value that is not a dynamic config item.
ConfigId RegisterInternal(std::string&& name, Factory factory);

/// @brief Returns metadata of every config item registered in this binary.
/// Repeated registrations of the same name are preserved, not deduplicated.
/// Must be called after static initialization completes.
std::vector<dynamic_config::RegisteredConfigMeta> GetRegisteredConfigsMeta();

struct InternalTag final {
    explicit InternalTag() = default;
};

std::any MakeConfig(ConfigId id, const DocsMap&);

std::string_view GetName(ConfigId id);

DocsMap MakeDefaultDocsMap();

struct ConfigIdGetter final {
    template <typename Key>
    static ConfigId Get(const Key& key) noexcept {
        return key.id_;
    }
};

class SnapshotData final {
public:
    SnapshotData() = default;

    explicit SnapshotData(const std::vector<KeyValue>& config_variables);

    SnapshotData(const DocsMap& defaults, const std::vector<KeyValue>& overrides);

    SnapshotData(const SnapshotData& defaults, const std::vector<KeyValue>& overrides);

    SnapshotData(SnapshotData&&) noexcept = default;
    SnapshotData& operator=(SnapshotData&&) noexcept = default;

    template <typename T>
    const T& Get(ConfigId id) const {
        try {
            return std::any_cast<const T&>(DoGet(id));
        } catch (const std::exception& ex) {
            impl::WrapGetError(ex, typeid(T));
        }
    }

    bool IsEmpty() const noexcept;

private:
    const std::any& DoGet(ConfigId id) const;

    std::vector<std::any> user_configs_;
};

class StorageData;

}  // namespace dynamic_config::impl

USERVER_NAMESPACE_END
