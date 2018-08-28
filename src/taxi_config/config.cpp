#include <taxi_config/config.hpp>

#include <utils/demangle.hpp>

namespace taxi_config {

namespace {
std::unordered_map<std::type_index, std::function<boost::any(const DocsMap&)>>&
ExtraConfigFactories() {
  static std::unordered_map<std::type_index,
                            std::function<boost::any(const DocsMap&)>>
      factories;
  return factories;
}
}  // namespace

Config::Config(Config&&) = default;

Config::~Config() = default;

Config::Config(const DocsMap& docs_map) {
  for (const auto& extra_config : ExtraConfigFactories()) {
    extra_configs_[extra_config.first] = extra_config.second(docs_map);
  }
}

void Config::Register(const std::type_info& type,
                      std::function<boost::any(const DocsMap&)>&& factory) {
  ExtraConfigFactories()[type] = std::move(factory);
}

const boost::any& Config::Get(const std::type_index& type) const {
  try {
    return extra_configs_.at(type);
  } catch (const std::out_of_range& ex) {
    throw std::out_of_range("Type " + utils::GetTypeName(type) +
                            " is not registered as config");
  }
}

}  // namespace taxi_config
