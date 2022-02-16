#include <userver/dynamic_config/snapshot_impl.hpp>

#include <atomic>

#include <fmt/format.h>

#include <userver/compiler/demangle.hpp>
#include <userver/dynamic_config/storage_mock.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/cpu_relax.hpp>
#include <userver/utils/enumerate.hpp>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config::impl {
namespace {

std::vector<impl::Factory>& Registry() {
  static std::vector<impl::Factory> registry;
  return registry;
}

std::atomic<bool> is_config_registration_allowed{true};

}  // namespace

[[noreturn]] void WrapGetError(const std::exception& ex, std::type_index type) {
  throw std::logic_error(fmt::format("Error in Config::Get<{}>: {}",
                                     compiler::GetTypeName(type), ex.what()));
}

impl::ConfigId Register(impl::Factory factory) {
  auto& registry = Registry();
  registry.push_back(factory);
  UASSERT_MSG(is_config_registration_allowed,
              "Config registry modification is disallowed at this stage: "
              "configs are already being constructed");
  return registry.size() - 1;
}

SnapshotData::SnapshotData(const std::vector<KeyValue>& config_variables) {
  is_config_registration_allowed = false;
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
      user_configs_[id] = factory(defaults);
    }
  }
}

SnapshotData::SnapshotData(const SnapshotData& defaults,
                           const std::vector<KeyValue>& overrides)
    : user_configs_(defaults.user_configs_) {
  for (auto& config_variable : overrides) {
    user_configs_[config_variable.GetId()] = config_variable.GetValue();
  }
}

const std::any& SnapshotData::Get(impl::ConfigId id) const {
  const auto& config = user_configs_[id];
  if (!config.has_value()) {
    throw std::logic_error("This type is not registered as config");
  }
  return config;
}

}  // namespace dynamic_config::impl

USERVER_NAMESPACE_END
