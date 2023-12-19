#pragma once

#include <string>
#include <unordered_set>

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace baggage {

struct BaggageSettings final {
  std::unordered_set<std::string> allowed_keys;
};

BaggageSettings Parse(const formats::json::Value& value,
                      formats::parse::To<BaggageSettings>);

extern const dynamic_config::Key<BaggageSettings> kBaggageSettings;

extern const dynamic_config::Key<bool> kBaggageEnabled;

}  // namespace baggage

USERVER_NAMESPACE_END
