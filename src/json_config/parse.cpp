#include "parse.hpp"

namespace json_config {

ParseError::ParseError(const std::string& full_path, const std::string& name,
                       const std::string& type)
    : std::runtime_error("Cannot parse " + full_path +
                         (full_path.empty() ? "" : ".") + name + ", " + type +
                         " expected") {}

namespace impl {

namespace {

enum class ValueKind { kOptional, kRequired };

boost::optional<int> ParseIntImpl(const Json::Value& obj,
                                  const std::string& name,
                                  const std::string& full_path,
                                  ValueKind kind) {
  const auto& value = obj[name];
  if (kind == ValueKind::kOptional && value.isNull()) {
    return {};
  }
  if (!value.isInt()) {
    throw ParseError(full_path, name, "int");
  }
  return value.asInt();
}

boost::optional<bool> ParseBoolImpl(const Json::Value& obj,
                                    const std::string& name,
                                    const std::string& full_path,
                                    ValueKind kind) {
  const auto& value = obj[name];
  if (kind == ValueKind::kOptional && value.isNull()) {
    return {};
  }
  if (!value.isBool()) {
    throw ParseError(full_path, name, "bool");
  }
  return value.asBool();
}

boost::optional<uint64_t> ParseUint64Impl(const Json::Value& obj,
                                          const std::string& name,
                                          const std::string& full_path,
                                          ValueKind kind) {
  const auto& value = obj[name];
  if (kind == ValueKind::kOptional && value.isNull()) {
    return {};
  }
  if (!value.isUInt64()) {
    throw ParseError(full_path, name, "uint64");
  }
  return value.asUInt64();
}

boost::optional<std::string> ParseStringImpl(const Json::Value& obj,
                                             const std::string& name,
                                             const std::string& full_path,
                                             ValueKind kind) {
  const auto& value = obj[name];
  if (kind == ValueKind::kOptional && value.isNull()) {
    return {};
  }
  if (!value.isString()) {
    throw ParseError(full_path, name, "string");
  }
  return value.asString();
}

}  // namespace

int ParseInt(const Json::Value& obj, const std::string& name,
             const std::string& full_path) {
  return *ParseIntImpl(obj, name, full_path, ValueKind::kRequired);
}

bool ParseBool(const Json::Value& obj, const std::string& name,
               const std::string& full_path) {
  return *ParseBoolImpl(obj, name, full_path, ValueKind::kRequired);
}

uint64_t ParseUint64(const Json::Value& obj, const std::string& name,
                     const std::string& full_path) {
  return *ParseUint64Impl(obj, name, full_path, ValueKind::kRequired);
}

std::string ParseString(const Json::Value& obj, const std::string& name,
                        const std::string& full_path) {
  return std::move(
      *ParseStringImpl(obj, name, full_path, ValueKind::kRequired));
}

boost::optional<int> ParseOptionalInt(const Json::Value& obj,
                                      const std::string& name,
                                      const std::string& full_path) {
  return ParseIntImpl(obj, name, full_path, ValueKind::kOptional);
}

boost::optional<bool> ParseOptionalBool(const Json::Value& obj,
                                        const std::string& name,
                                        const std::string& full_path) {
  return ParseBoolImpl(obj, name, full_path, ValueKind::kOptional);
}

boost::optional<uint64_t> ParseOptionalUint64(const Json::Value& obj,
                                              const std::string& name,
                                              const std::string& full_path) {
  return ParseUint64Impl(obj, name, full_path, ValueKind::kOptional);
}

boost::optional<std::string> ParseOptionalString(const Json::Value& obj,
                                                 const std::string& name,
                                                 const std::string& full_path) {
  return ParseStringImpl(obj, name, full_path, ValueKind::kOptional);
}

}  // namespace impl
}  // namespace json_config
