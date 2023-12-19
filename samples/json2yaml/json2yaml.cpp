/// [json2yaml - includes]
#include <iostream>
#include <string>

#include <userver/formats/json.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/formats/yaml.hpp>
/// [json2yaml - includes]

/// [json2yaml - convert]
USERVER_NAMESPACE_BEGIN

namespace formats::parse {

yaml::Value Convert(const json::Value& json, parse::To<yaml::Value>) {
  yaml::ValueBuilder yaml_vb;

  if (json.IsBool()) {
    yaml_vb = json.ConvertTo<bool>();
  } else if (json.IsInt64()) {
    yaml_vb = json.ConvertTo<int64_t>();
  } else if (json.IsUInt64()) {
    yaml_vb = json.ConvertTo<uint64_t>();
  } else if (json.IsDouble()) {
    yaml_vb = json.ConvertTo<double>();
  } else if (json.IsString()) {
    yaml_vb = json.ConvertTo<std::string>();
  } else if (json.IsArray()) {
    yaml_vb = {common::Type::kArray};
    for (const auto& elem : json) {
      yaml_vb.PushBack(elem.ConvertTo<yaml::Value>());
    }
  } else if (json.IsObject()) {
    yaml_vb = {common::Type::kObject};
    for (auto it = json.begin(); it != json.end(); ++it) {
      yaml_vb[it.GetName()] = it->ConvertTo<yaml::Value>();
    }
  }

  return yaml_vb.ExtractValue();
}

}  // namespace formats::parse

USERVER_NAMESPACE_END
/// [json2yaml - convert]

/// [json2yaml - main]
int main() {
  namespace formats = USERVER_NAMESPACE::formats;

  auto json = formats::json::FromStream(std::cin);
  formats::yaml::Serialize(json.ConvertTo<formats::yaml::Value>(), std::cout);
  std::cout << std::endl;
}
/// [json2yaml - main]
