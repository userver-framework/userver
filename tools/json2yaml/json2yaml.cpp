#include <iostream>
#include <string>

#include <formats/json.hpp>
#include <formats/parse/to.hpp>
#include <formats/yaml.hpp>

namespace formats::parse {

static formats::yaml::Value Convert(const formats::json::Value& json,
                                    formats::parse::To<formats::yaml::Value>) {
  formats::yaml::ValueBuilder yaml_vb;

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
    yaml_vb = {formats::common::Type::kArray};
    for (const auto& elem : json) {
      yaml_vb.PushBack(elem.ConvertTo<formats::yaml::Value>());
    }
  } else if (json.IsObject()) {
    yaml_vb = {formats::common::Type::kObject};
    for (auto it = json.begin(); it != json.end(); ++it) {
      yaml_vb[it.GetName()] = it->ConvertTo<formats::yaml::Value>();
    }
  }

  return yaml_vb.ExtractValue();
}

}  // namespace formats::parse

int main() {
  formats::yaml::Serialize(
      formats::json::FromStream(std::cin).ConvertTo<formats::yaml::Value>(),
      std::cout);
  std::cout << std::endl;
}
