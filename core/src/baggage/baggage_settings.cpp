#include <userver/baggage/baggage_settings.hpp>

#include <userver/dynamic_config/value.hpp>
#include <userver/formats/json/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

namespace baggage {

BaggageSettings Parse(const formats::json::Value& value,
                      formats::parse::To<BaggageSettings>) {
  BaggageSettings result{};
  result.allowed_keys =
      value["allowed_keys"].As<std::unordered_set<std::string>>();
  return result;
}

const dynamic_config::Key<BaggageSettings> kBaggageSettings{
    "BAGGAGE_SETTINGS",
    dynamic_config::DefaultAsJsonString{R"({"allowed_keys": []})"},
};

const dynamic_config::Key<bool> kBaggageEnabled{"USERVER_BAGGAGE_ENABLED",
                                                false};

}  // namespace baggage

USERVER_NAMESPACE_END
