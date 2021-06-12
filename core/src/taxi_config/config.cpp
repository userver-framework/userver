#include <taxi_config/config.hpp>

#include <atomic>

#include <fmt/format.h>

#include <compiler/demangle.hpp>
#include <taxi_config/storage_mock.hpp>
#include <utils/assert.hpp>
#include <utils/enumerate.hpp>

namespace taxi_config {

namespace {

std::vector<impl::Factory>& Registry() {
  static std::vector<impl::Factory> registry;
  return registry;
}

std::atomic<bool> is_config_registration_allowed{true};

}  // namespace

namespace impl {

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

}  // namespace impl

Config::Config(const DocsMap& docs_map) {
  is_config_registration_allowed = false;
  user_configs_.reserve(Registry().size());

  for (auto* factory : Registry()) {
    if (factory != nullptr) {
      user_configs_.push_back(factory(docs_map));
    } else {
      user_configs_.emplace_back();
    }
  }
}

Config::Config(const std::vector<KeyValue>& config_variables) {
  is_config_registration_allowed = false;
  user_configs_.resize(Registry().size());

  for (const auto& config_variable : config_variables) {
    config_variable.Store(user_configs_);
  }
}

Config::Config(const DocsMap& defaults, const std::vector<KeyValue>& overrides)
    : Config(overrides) {
  for (const auto [id, factory] : utils::enumerate(Registry())) {
    if (!user_configs_[id].has_value()) {
      user_configs_[id] = factory(defaults);
    }
  }
}

Config::Config(const Config& defaults, const std::vector<KeyValue>& overrides)
    : user_configs_(defaults.user_configs_) {
  for (auto& config_variable : overrides) {
    config_variable.Store(user_configs_);
  }
}

Config Config::Parse(const DocsMap& docs_map) { return Config{docs_map}; }

const std::any& Config::Get(impl::ConfigId id) const {
  const auto& config = user_configs_[id];
  if (!config.has_value()) {
    throw std::logic_error("This type is not registered as config");
  }
  return config;
}

}  // namespace taxi_config
