#include <taxi_config/config.hpp>

#include <compiler/demangle.hpp>
#include <utils/assert.hpp>

namespace taxi_config {

template <typename BaseConfigTag>
std::unordered_map<std::type_index, std::function<boost::any(const DocsMap&)>>&
ExtraBaseConfigFactories() {
  static std::unordered_map<std::type_index,
                            std::function<boost::any(const DocsMap&)>>
      factories;
  return factories;
}

template <typename ConfigTag>
BaseConfig<ConfigTag>::BaseConfig(const DocsMap& docs_map) {
  for (const auto& extra_config : ExtraBaseConfigFactories<ConfigTag>()) {
    extra_configs_[extra_config.first] = extra_config.second(docs_map);
  }
}

template <typename ConfigTag>
void BaseConfig<ConfigTag>::DoRegister(
    const std::type_info& type,
    std::function<boost::any(const DocsMap&)>&& factory) {
  ExtraBaseConfigFactories<ConfigTag>()[type] = std::move(factory);
}

template <typename ConfigTag>
void BaseConfig<ConfigTag>::Unregister(const std::type_info& type) {
  [[maybe_unused]] size_t count =
      ExtraBaseConfigFactories<ConfigTag>().erase(type);
  UASSERT(count == 1);
}

template <typename ConfigTag>
const boost::any& BaseConfig<ConfigTag>::Get(
    const std::type_index& type) const {
  try {
    return extra_configs_.at(type);
  } catch (const std::out_of_range& ex) {
    throw std::out_of_range("Type " + compiler::GetTypeName(type) +
                            " is not registered as config");
  }
}

template class BaseConfig<FullConfigTag>;
template class BaseConfig<BootstrapConfigTag>;

}  // namespace taxi_config
