#pragma once

#include <any>
#include <cstddef>
#include <exception>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <vector>

#include <userver/dynamic_config/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config::impl {

using Factory = std::any (*)(const DocsMap&);

template <typename Key>
using VariableOfKey = decltype(Key::Parse(std::declval<const DocsMap&>()));

template <typename Key>
std::any FactoryFor(const DocsMap& map) {
  return std::any{Key::Parse(map)};
}

[[noreturn]] void WrapGetError(const std::exception& ex, std::type_index type);

template <typename T>
T ParseByConstructor(const DocsMap& docs_map) {
  return T(docs_map);
}

using ConfigId = std::size_t;

ConfigId Register(Factory factory);

// Automatically registers all used config types at startup and assigns them
// sequential ids
template <typename Key>
inline const ConfigId kConfigId = Register(&FactoryFor<Key>);

class SnapshotData final {
 public:
  SnapshotData() = default;

  SnapshotData(const std::vector<KeyValue>& config_variables);

  SnapshotData(const DocsMap& defaults, const std::vector<KeyValue>& overrides);

  SnapshotData(const SnapshotData& defaults,
               const std::vector<KeyValue>& overrides);

  SnapshotData(SnapshotData&&) noexcept = default;
  SnapshotData& operator=(SnapshotData&&) noexcept = default;

  template <typename Key>
  const auto& operator[](Key) const {
    using VariableType = VariableOfKey<Key>;
    try {
      return std::any_cast<const VariableType&>(Get(impl::kConfigId<Key>));
    } catch (const std::exception& ex) {
      impl::WrapGetError(ex, typeid(VariableType));
    }
  }

  bool IsEmpty() const noexcept;

 private:
  const std::any& Get(impl::ConfigId id) const;

  std::vector<std::any> user_configs_;
};

class StorageData;

}  // namespace dynamic_config::impl

USERVER_NAMESPACE_END
