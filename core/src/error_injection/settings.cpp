#include <error_injection/settings.hpp>

#include <unordered_map>

#include <formats/yaml/value.hpp>
#include <logging/log.hpp>

namespace error_injection {

Verdict Parse(const yaml_config::YamlConfig& yaml,
              formats::parse::To<Verdict>) {
  // No skip
  static const std::unordered_map<std::string, Verdict> verdicts_by_name = {
      {"timeout", Verdict::Timeout},
      {"error", Verdict::Error},
      {"max-delay", Verdict::MaxDelay},
      {"random-delay", Verdict::RandomDelay},
  };

  const auto& value = yaml.As<std::string>();
  auto it = verdicts_by_name.find(value);
  if (it != verdicts_by_name.end()) return it->second;

  throw std::runtime_error("can't parse error_injection::Verdict from '" +
                           value + '\'');
}

Settings Parse(const yaml_config::YamlConfig& value,
               formats::parse::To<Settings>) {
  Settings settings;
  settings.enabled = value["enabled"].As<bool>();
  settings.probability = value["probability"].As<double>();
  settings.possible_verdicts = value["verdicts"].As<std::vector<Verdict>>();

  LOG_DEBUG() << "enabled = " << settings.enabled
              << " probability = " << settings.probability;

  return settings;
}

}  // namespace error_injection
