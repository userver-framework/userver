#include <yaml_config/parse.hpp>

#include <fmt/compile.h>

namespace yaml_config {
namespace impl {

void CheckIsMap(const formats::yaml::Value& obj, std::string_view full_path) {
  if (!obj.IsObject()) throw ParseError({}, full_path, "map");
}

void CheckIsSequence(const formats::yaml::Value& obj,
                     std::string_view full_path) {
  if (!obj.IsArray()) throw ParseError({}, full_path, "sequence");
}

void CheckContainer(const formats::yaml::Value& obj, std::string_view,
                    std::string_view full_path) {
  return CheckIsMap(obj, full_path);
}

void CheckContainer(const formats::yaml::Value& obj, size_t,
                    std::string_view full_path) {
  return CheckIsSequence(obj, full_path);
}

}  // namespace impl
}  // namespace yaml_config
