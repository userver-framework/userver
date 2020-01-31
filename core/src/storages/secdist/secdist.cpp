#include <storages/secdist/secdist.hpp>

#include <cerrno>
#include <fstream>
#include <sstream>

#include <compiler/demangle.hpp>
#include <formats/json/exception.hpp>
#include <formats/json/serialize.hpp>
#include <formats/json/value_builder.hpp>
#include <storages/secdist/exceptions.hpp>
#include <yaml_config/value.hpp>

namespace storages::secdist {

namespace {

std::vector<std::function<boost::any(const formats::json::Value&)>>&
GetConfigFactories() {
  static std::vector<std::function<boost::any(const formats::json::Value&)>>
      factories;
  return factories;
}

}  // namespace

SecdistConfig::SecdistConfig() = default;

SecdistConfig::SecdistConfig(const std::string& path) {
  // if we don't want to read secdist, then we don't need to initialize
  if (GetConfigFactories().empty()) return;

  formats::json::Value doc;

  std::ifstream json_stream(path);
  try {
    doc = formats::json::FromStream(json_stream);
  } catch (const std::exception& e) {
    throw SecdistError(
        "Cannot load secdist config. File '" + path +
        "' doesn't exist, unrechable or in invalid format:" + e.what());
  }

  Init(doc);
}

void SecdistConfig::Init(const formats::json::Value& doc) {
  for (const auto& config_factory : GetConfigFactories()) {
    configs_.emplace_back(config_factory(doc));
  }
}

std::size_t SecdistConfig::Register(
    std::function<boost::any(const formats::json::Value&)>&& factory) {
  auto& config_factories = GetConfigFactories();
  config_factories.emplace_back(std::move(factory));
  return config_factories.size() - 1;
}

const boost::any& SecdistConfig::Get(const std::type_index& type,
                                     std::size_t index) const {
  try {
    return configs_.at(index);
  } catch (const std::out_of_range&) {
    throw std::out_of_range("Type " + compiler::GetTypeName(type) +
                            " is not registered as config");
  }
}

}  // namespace storages::secdist
