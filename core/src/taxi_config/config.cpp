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

template <typename ConfigTag>
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

template <typename ConfigTag>
BaseConfig<ConfigTag>::BaseConfig(const DocsMap& docs_map) {
  is_config_registration_allowed = false;
  user_configs_.reserve(Registry<ConfigTag>().size());

  for (auto* factory : Registry<ConfigTag>()) {
    if (factory != nullptr) {
      user_configs_.push_back(factory(docs_map));
    } else {
      user_configs_.emplace_back();
    }
  }
}

template <typename ConfigTag>
BaseConfig<ConfigTag> BaseConfig<ConfigTag>::Parse(const DocsMap& docs_map) {
  return BaseConfig{docs_map};
}

template <typename ConfigTag>
auto BaseConfig<ConfigTag>::Register(impl::Factory factory) -> ConfigId {
  Registry<ConfigTag>().push_back(factory);
  AssertRegistrationAllowed();
  return Registry<ConfigTag>().size() - 1;
}

template <typename ConfigTag>
bool BaseConfig<ConfigTag>::IsRegistered(ConfigId id) {
  UASSERT(id < Registry<ConfigTag>().size());
  return Registry<ConfigTag>()[id] != nullptr;
}

template <typename ConfigTag>
void BaseConfig<ConfigTag>::Unregister(ConfigId id) {
  UASSERT(IsRegistered(id));
  Registry<ConfigTag>()[id] = nullptr;
  AssertRegistrationAllowed();
}

template <typename ConfigTag>
const std::any& BaseConfig<ConfigTag>::Get(ConfigId id) const {
  const auto& config = user_configs_[id];
  if (!config.has_value()) {
    throw std::logic_error("This type is not registered as config");
  }
  return config;
}

template class BaseConfig<FullConfigTag>;
template class BaseConfig<BootstrapConfigTag>;

}  // namespace taxi_config
