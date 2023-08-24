#include <userver/formats/yaml/serialize.hpp>

#include <array>
#include <fstream>
#include <memory>
#include <sstream>

#include <fmt/format.h>

#include <userver/formats/yaml/exception.hpp>

#include <yaml-cpp/yaml.h>

USERVER_NAMESPACE_BEGIN

namespace formats::yaml {

formats::yaml::Value FromString(const std::string& doc) {
  if (doc.empty()) {
    throw ParseException("YAML document is empty");
  }

  try {
    return YAML::Load(doc);
  } catch (const YAML::ParserException& e) {
    throw ParseException(e.what());
  }
}

formats::yaml::Value FromStream(std::istream& is) {
  if (!is) {
    throw BadStreamException(is);
  }

  try {
    return YAML::Load(is);
  } catch (const YAML::ParserException& e) {
    throw ParseException(e.what());
  }
}

void Serialize(const formats::yaml::Value& doc, std::ostream& os) {
  os << doc.GetNative();
  if (!os) {
    throw BadStreamException(os);
  }
}

std::string ToString(const formats::yaml::Value& doc) {
  std::ostringstream os;
  Serialize(doc, os);
  return os.str();
}

namespace blocking {
formats::yaml::Value FromFile(const std::string& path) {
  std::ifstream is(path);
  try {
    return FromStream(is);
  } catch (const std::exception& e) {
    throw ParseException(
        fmt::format("Parsing '{}' failed. {}", path, e.what()));
  }
}
}  // namespace blocking

}  // namespace formats::yaml

USERVER_NAMESPACE_END
