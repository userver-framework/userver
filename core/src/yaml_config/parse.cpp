#include <yaml_config/parse.hpp>

namespace yaml_config {
namespace impl {

std::string PathAppend(const std::string& full_path, const std::string& name) {
  return full_path + (full_path.empty() ? "" : ".") + name;
}

std::string PathAppend(const std::string& full_path, size_t idx) {
  return full_path + '[' + std::to_string(idx) + ']';
}

void CheckIsMap(const formats::yaml::Value& obj, const std::string& full_path) {
  if (!obj.IsObject()) throw ParseError({}, full_path, "map");
}

void CheckIsSequence(const formats::yaml::Value& obj,
                     const std::string& full_path) {
  if (!obj.IsArray()) throw ParseError({}, full_path, "sequence");
}

void CheckContainer(const formats::yaml::Value& obj, const std::string&,
                    const std::string& full_path) {
  return CheckIsMap(obj, full_path);
}

void CheckContainer(const formats::yaml::Value& obj, const size_t&,
                    const std::string& full_path) {
  return CheckIsSequence(obj, full_path);
}

}  // namespace impl
}  // namespace yaml_config
