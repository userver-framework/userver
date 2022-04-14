#include <userver/error_injection/settings.hpp>

#include <unordered_map>

#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/yaml/value.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/consteval_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace error_injection {

// No skip
constexpr auto kVerdictsByName =
    utils::MakeConsinitMap<std::string_view, Verdict>({
        {"timeout", Verdict::Timeout},
        {"error", Verdict::Error},
        {"max-delay", Verdict::MaxDelay},
        {"random-delay", Verdict::RandomDelay},
    });

Verdict Parse(const yaml_config::YamlConfig& yaml,
              formats::parse::To<Verdict>) {
  const auto& value = yaml.As<std::string>();
  auto ptr = kVerdictsByName.FindOrNullptr(value);
  if (ptr) return *ptr;

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

USERVER_NAMESPACE_END
