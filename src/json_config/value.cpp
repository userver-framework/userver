#include <yandex/taxi/userver/json_config/value.hpp>

namespace json_config {
namespace impl {

bool IsSubstitution(const Json::Value& value) {
  if (!value.isString()) {
    return false;
  }
  const auto& str = value.asString();
  return !str.empty() && str.front() == '$';
}

std::string GetSubstitutionVarName(const Json::Value& value) {
  return value.asString().substr(1);
}

std::string GetFallbackName(const std::string& str) {
  return str + "#fallback";
}

}  // namespace impl

namespace {

template <typename ParserImpl>
auto AdaptImpl(ParserImpl parser_impl) {
  return [parser_impl = std::move(parser_impl)](
      const Json::Value& obj, const std::string& name,
      const std::string& full_path, const VariableMapPtr&) {
    return parser_impl(obj, name, full_path);
  };
}

}  // namespace

void CheckIsObject(const Json::Value& obj, const std::string& full_path) {
  impl::CheckIsObject(obj, full_path);
}

int ParseInt(const Json::Value& obj, const std::string& name,
             const std::string& full_path,
             const VariableMapPtr& config_vars_ptr) {
  auto optional = ParseOptionalInt(obj, name, full_path, config_vars_ptr);
  if (!optional) {
    throw ParseError(full_path, name, "int");
  }
  return *optional;
}

bool ParseBool(const Json::Value& obj, const std::string& name,
               const std::string& full_path,
               const VariableMapPtr& config_vars_ptr) {
  auto optional = ParseOptionalBool(obj, name, full_path, config_vars_ptr);
  if (!optional) {
    throw ParseError(full_path, name, "bool");
  }
  return *optional;
}

uint64_t ParseUint64(const Json::Value& obj, const std::string& name,
                     const std::string& full_path,
                     const VariableMapPtr& config_vars_ptr) {
  auto optional = ParseOptionalUint64(obj, name, full_path, config_vars_ptr);
  if (!optional) {
    throw ParseError(full_path, name, "uint64");
  }
  return *optional;
}

std::string ParseString(const Json::Value& obj, const std::string& name,
                        const std::string& full_path,
                        const VariableMapPtr& config_vars_ptr) {
  auto optional = ParseOptionalString(obj, name, full_path, config_vars_ptr);
  if (!optional) {
    throw ParseError(full_path, name, "string");
  }
  return std::move(*optional);
}

boost::optional<int> ParseOptionalInt(const Json::Value& obj,
                                      const std::string& name,
                                      const std::string& full_path,
                                      const VariableMapPtr& config_vars_ptr) {
  return ParseValue(obj, name, full_path, config_vars_ptr,
                    AdaptImpl(&impl::ParseInt), &ParseOptionalInt);
}

boost::optional<bool> ParseOptionalBool(const Json::Value& obj,
                                        const std::string& name,
                                        const std::string& full_path,
                                        const VariableMapPtr& config_vars_ptr) {
  return ParseValue(obj, name, full_path, config_vars_ptr,
                    AdaptImpl(&impl::ParseBool), &ParseOptionalBool);
}

boost::optional<uint64_t> ParseOptionalUint64(
    const Json::Value& obj, const std::string& name,
    const std::string& full_path, const VariableMapPtr& config_vars_ptr) {
  return ParseValue(obj, name, full_path, config_vars_ptr,
                    AdaptImpl(&impl::ParseUint64), &ParseOptionalUint64);
}

boost::optional<std::string> ParseOptionalString(
    const Json::Value& obj, const std::string& name,
    const std::string& full_path, const VariableMapPtr& config_vars_ptr) {
  return ParseValue(obj, name, full_path, config_vars_ptr,
                    AdaptImpl(&impl::ParseString), &ParseOptionalString);
}

}  // namespace json_config
