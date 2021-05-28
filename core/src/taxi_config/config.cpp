#include <taxi_config/config.hpp>

#include <atomic>

#include <fmt/format.h>

#include <compiler/demangle.hpp>
#include <utils/assert.hpp>

namespace taxi_config {

namespace impl {

[[noreturn]] void WrapGetError(const std::exception& ex, std::type_index type) {
  throw std::logic_error(fmt::format("Error in Config::Get<{}>: {}",
                                     compiler::GetTypeName(type), ex.what()));
}

}  // namespace impl

namespace {

std::vector<impl::Factory>& Registry() {
  static std::vector<impl::Factory> registry;
  return registry;
}

std::atomic<bool> is_config_registration_allowed{true};

void AssertRegistrationAllowed() {
  UASSERT_MSG(is_config_registration_allowed,
              "Config registry modification is disallowed at this stage: "
              "configs are already being constructed");
}

}  // namespace

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

Config::Config(std::initializer_list<KeyValue> config_variables)
    : user_configs_(Registry().size()) {
  for (auto& config_variable : config_variables) {
    user_configs_[config_variable.id_] = config_variable.value_;
  }
}

Config Config::Parse(const DocsMap& docs_map) { return Config{docs_map}; }

auto Config::Register(impl::Factory factory) -> ConfigId {
  Registry().push_back(factory);
  AssertRegistrationAllowed();
  return Registry().size() - 1;
}

bool Config::IsRegistered(ConfigId id) {
  UASSERT(id < Registry().size());
  return Registry()[id] != nullptr;
}

void Config::Unregister(ConfigId id) {
  UASSERT(IsRegistered(id));
  Registry()[id] = nullptr;
  AssertRegistrationAllowed();
}

const std::any& Config::Get(ConfigId id) const {
  const auto& config = user_configs_[id];
  if (!config.has_value()) {
    throw std::logic_error("This type is not registered as config");
  }
  return config;
}

}  // namespace taxi_config
