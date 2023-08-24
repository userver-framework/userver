#include <userver/dynamic_config/impl/snapshot.hpp>

#include <fmt/format.h>

#include <userver/compiler/demangle.hpp>
#include <userver/dynamic_config/storage_mock.hpp>
#include <userver/utils/cpu_relax.hpp>
#include <userver/utils/enumerate.hpp>
#include <userver/utils/impl/static_registration.hpp>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config::impl {
namespace {

std::vector<impl::Factory>& Registry() {
  static std::vector<impl::Factory> registry;
  return registry;
}

}  // namespace

[[noreturn]] void WrapGetError(const std::exception& ex, std::type_index type) {
  throw std::logic_error(fmt::format("Error in Config::Get<{}>: {}",
                                     compiler::GetTypeName(type), ex.what()));
}

impl::ConfigId Register(impl::Factory factory) {
  utils::impl::AssertStaticRegistrationAllowed(
      "dynamic_config::Key registration");
  auto& registry = Registry();
  registry.push_back(factory);
  return registry.size() - 1;
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
  for (const auto [id, factory] : utils::enumerate(Registry())) {
    if (!user_configs_[id].has_value()) {
      relax.Relax(1);
      try {
        user_configs_[id] = factory(defaults);
      } catch (const std::exception& ex) {
        throw std::runtime_error(
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

const std::any& SnapshotData::Get(impl::ConfigId id) const {
  UASSERT_MSG(id < user_configs_.size(), "SnapshotData is in an empty state.");
  const auto& config = user_configs_[id];
  if (!config.has_value()) {
    throw std::logic_error("This type is not registered as config");
  }
  return config;
}

}  // namespace dynamic_config::impl

USERVER_NAMESPACE_END
