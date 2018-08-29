#include <storages/secdist/secdist.hpp>

#include <cerrno>
#include <fstream>
#include <sstream>

#include <json/reader.h>
#include <json_config/value.hpp>
#include <utils/demangle.hpp>
#include "exceptions.hpp"

#include <json_config/value.hpp>

namespace storages {
namespace secdist {

namespace {

std::vector<std::function<boost::any(const Json::Value&)>>&
GetConfigFactories() {
  static std::vector<std::function<boost::any(const Json::Value&)>> factories;
  return factories;
}

}  // namespace

SecdistConfig::SecdistConfig() = default;

SecdistConfig::SecdistConfig(const std::string& path) {
  std::ifstream json_stream(path);

  if (!json_stream) {
    std::error_code ec(errno, std::system_category());
    throw InvalidSecdistJson("Cannot load secdist config: " + ec.message());
  }

  Json::Reader reader;
  Json::Value doc;

  if (!reader.parse(json_stream, doc)) {
    throw InvalidSecdistJson("Cannot load secdist config: " +
                             reader.getFormattedErrorMessages());
  }

  Init(doc);
}

void SecdistConfig::Init(const Json::Value& doc) {
  for (const auto& config_factory : GetConfigFactories()) {
    configs_.emplace_back(config_factory(doc));
  }
}

std::size_t SecdistConfig::Register(
    std::function<boost::any(const Json::Value&)>&& factory) {
  auto& config_factories = GetConfigFactories();
  config_factories.emplace_back(std::move(factory));
  return config_factories.size() - 1;
}

const boost::any& SecdistConfig::Get(const std::type_index& type,
                                     const std::size_t index) const {
  try {
    return configs_.at(index);
  } catch (const std::out_of_range&) {
    throw std::out_of_range("Type " + utils::GetTypeName(type) +
                            " is not registered as config");
  }
}

}  // namespace secdist
}  // namespace storages
