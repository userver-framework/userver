#include <yaml_config/parse.hpp>

namespace yaml_config {

ParseError::ParseError(const std::string& full_path, const std::string& name,
                       const std::string& type)
    : std::runtime_error("Cannot parse " + full_path +
                         (full_path.empty() ? "" : ".") + name + ", " + type +
                         " expected") {}

namespace impl {

namespace {

enum class ValueKind { kOptional, kRequired };

boost::optional<int> ParseIntImpl(const formats::yaml::Node& obj,
                                  const std::string& name,
                                  const std::string& full_path,
                                  ValueKind kind) {
  const auto& value = obj[name];
  if (kind == ValueKind::kOptional && !value) {
    return {};
  }
  CheckIsMap(obj, full_path);
  try {
    return value.as<int>();
  } catch (formats::yaml::Exception&) {
    throw ParseError(full_path, name, "int");
  }
}

boost::optional<bool> ParseBoolImpl(const formats::yaml::Node& obj,
                                    const std::string& name,
                                    const std::string& full_path,
                                    ValueKind kind) {
  const auto& value = obj[name];
  if (kind == ValueKind::kOptional && !value) {
    return {};
  }
  CheckIsMap(obj, full_path);
  try {
    return value.as<bool>();
  } catch (formats::yaml::Exception&) {
    throw ParseError(full_path, name, "bool");
  }
}

boost::optional<uint64_t> ParseUint64Impl(const formats::yaml::Node& obj,
                                          const std::string& name,
                                          const std::string& full_path,
                                          ValueKind kind) {
  const auto& value = obj[name];
  if (kind == ValueKind::kOptional && !value) {
    return {};
  }
  CheckIsMap(obj, full_path);
  try {
    return value.as<uint64_t>();
  } catch (formats::yaml::Exception&) {
    throw ParseError(full_path, name, "uint64");
  }
}

boost::optional<std::string> ParseStringImpl(const formats::yaml::Node& obj,
                                             const std::string& name,
                                             const std::string& full_path,
                                             ValueKind kind) {
  const auto& value = obj[name];
  if (kind == ValueKind::kOptional && !value) {
    return {};
  }
  CheckIsMap(obj, full_path);
  try {
    return value.as<std::string>();
  } catch (formats::yaml::Exception&) {
    throw ParseError(full_path, name, "string");
  }
}

}  // namespace

void CheckIsMap(const formats::yaml::Node& obj, const std::string& full_path) {
  if (!obj.IsMap()) throw ParseError({}, full_path, "map");
}

int ParseInt(const formats::yaml::Node& obj, const std::string& name,
             const std::string& full_path) {
  return *ParseIntImpl(obj, name, full_path, ValueKind::kRequired);
}

bool ParseBool(const formats::yaml::Node& obj, const std::string& name,
               const std::string& full_path) {
  return *ParseBoolImpl(obj, name, full_path, ValueKind::kRequired);
}

uint64_t ParseUint64(const formats::yaml::Node& obj, const std::string& name,
                     const std::string& full_path) {
  return *ParseUint64Impl(obj, name, full_path, ValueKind::kRequired);
}

std::string ParseString(const formats::yaml::Node& obj, const std::string& name,
                        const std::string& full_path) {
  return std::move(
      *ParseStringImpl(obj, name, full_path, ValueKind::kRequired));
}

boost::optional<int> ParseOptionalInt(const formats::yaml::Node& obj,
                                      const std::string& name,
                                      const std::string& full_path) {
  return ParseIntImpl(obj, name, full_path, ValueKind::kOptional);
}

boost::optional<bool> ParseOptionalBool(const formats::yaml::Node& obj,
                                        const std::string& name,
                                        const std::string& full_path) {
  return ParseBoolImpl(obj, name, full_path, ValueKind::kOptional);
}

boost::optional<uint64_t> ParseOptionalUint64(const formats::yaml::Node& obj,
                                              const std::string& name,
                                              const std::string& full_path) {
  return ParseUint64Impl(obj, name, full_path, ValueKind::kOptional);
}

boost::optional<std::string> ParseOptionalString(const formats::yaml::Node& obj,
                                                 const std::string& name,
                                                 const std::string& full_path) {
  return ParseStringImpl(obj, name, full_path, ValueKind::kOptional);
}

}  // namespace impl
}  // namespace yaml_config
