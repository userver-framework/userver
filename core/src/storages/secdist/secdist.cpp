#include <storages/secdist/secdist.hpp>

#include <cerrno>
#include <fstream>
#include <sstream>

#include <compiler/demangle.hpp>
#include <formats/json/exception.hpp>
#include <formats/json/serialize.hpp>
#include <storages/secdist/exceptions.hpp>
#include <yaml_config/value.hpp>

namespace storages {
namespace secdist {

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
  std::ifstream json_stream(path);
  formats::json::Value doc;
  try {
    doc = formats::json::FromStream(json_stream);
  } catch (const formats::json::Exception& e) {
    throw InvalidSecdistJson(std::string("Cannot load secdist config: ") +
                             e.what());
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
                                     const std::size_t index) const {
  try {
    return configs_.at(index);
  } catch (const std::out_of_range&) {
    throw std::out_of_range("Type " + compiler::GetTypeName(type) +
                            " is not registered as config");
  }
}

}  // namespace secdist
}  // namespace storages
