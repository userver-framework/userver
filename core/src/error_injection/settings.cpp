#include <userver/error_injection/settings.hpp>

#include <unordered_map>

#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/yaml/value.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/trivial_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace error_injection {

Verdict Parse(const yaml_config::YamlConfig& yaml,
              formats::parse::To<Verdict>) {
  const auto value = yaml.As<std::string>();
  static constexpr utils::TrivialBiMap kValues = [](auto selector) {
    return selector()
        .Case("timeout", Verdict::Timeout)
        .Case("error", Verdict::Error)
        .Case("max-delay", Verdict::MaxDelay)
        .Case("random-delay", Verdict::RandomDelay);
  };

  auto v = kValues.TryFind(value);
  if (v) return *v;

  throw std::runtime_error(fmt::format(
      "Can't parse error_injection::Verdict from '{}'. Expecting one of: {}",
      value, kValues.DescribeFirst()));
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

USERVER_NAMESPACE_END
