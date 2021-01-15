#include <taxi_config/config.hpp>

#include <compiler/demangle.hpp>
#include <utils/assert.hpp>

namespace taxi_config {

template <typename ConfigTag>
auto BaseConfig<ConfigTag>::ConfigFactories()
    -> std::unordered_map<std::type_index, Factory>& {
  static std::unordered_map<std::type_index, Factory> factories;
  return factories;
}

template <typename ConfigTag>
BaseConfig<ConfigTag>::BaseConfig(const DocsMap& docs_map) {
  for (const auto& extra_config : ConfigFactories()) {
    user_configs_[extra_config.first] = extra_config.second(docs_map);
  }
}

template <typename ConfigTag>
void BaseConfig<ConfigTag>::Register(std::type_index type, Factory factory) {
  ConfigFactories()[type] = factory;
}

template <typename ConfigTag>
bool BaseConfig<ConfigTag>::IsRegistered(std::type_index type) {
  return ConfigFactories().count(type) == 1;
}

template <typename ConfigTag>
void BaseConfig<ConfigTag>::Unregister(std::type_index type) {
  UASSERT(IsRegistered(type));
  ConfigFactories().erase(type);
}

template <typename ConfigTag>
const std::any& BaseConfig<ConfigTag>::Get(std::type_index type) const {
  try {
    return user_configs_.at(type);
  } catch (const std::out_of_range& ex) {
    throw std::out_of_range("Type " + compiler::GetTypeName(type) +
                            " is not registered as config");
  }
}

template class BaseConfig<FullConfigTag>;
template class BaseConfig<BootstrapConfigTag>;

}  // namespace taxi_config
