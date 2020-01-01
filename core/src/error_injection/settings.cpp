#include <error_injection/settings.hpp>

#include <unordered_map>

#include <formats/yaml/value.hpp>
#include <yaml_config/value.hpp>
#include <yaml_config/variable_map.hpp>

namespace error_injection {

Verdict Parse(const formats::yaml::Value& yaml, formats::parse::To<Verdict>) {
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

Settings Settings::ParseFromYaml(
    const formats::yaml::Value& yaml, const std::string& full_path,
    const yaml_config::VariableMapPtr& config_vars_ptr) {
  Settings settings;
  yaml_config::ParseInto(settings.enabled, yaml, "enabled", full_path,
                         config_vars_ptr);

  yaml_config::ParseInto(settings.probability, yaml, "probability", full_path,
                         config_vars_ptr);

  yaml_config::ParseInto(settings.possible_verdicts, yaml, "verdicts",
                         full_path, config_vars_ptr);

  LOG_ERROR() << "enabled = " << settings.enabled
              << " probability = " << settings.probability;

  return settings;
}

}  // namespace error_injection
