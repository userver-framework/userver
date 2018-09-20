#include <formats/json/serialize.hpp>

#include <array>
#include <fstream>
#include <memory>
#include <sstream>

#include <json/reader.h>
#include <json/writer.h>

#include <formats/json/exception.hpp>
#include <formats/json/value_detail.hpp>

namespace {
constexpr size_t kBufSize = 32 * 1024ull;
}  // namespace

namespace formats {
namespace json {

formats::json::Value FromString(const std::string& doc) {
  if (doc.empty()) {
    throw ParseException("JSON document is empty");
  }

  Json::CharReaderBuilder builder;
  builder["allowComments"] = false;

  // TODO: think of using reader pool
  std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
  auto root = std::make_shared<Json::Value>();
  std::string errors;
  if (!reader->parse(doc.data(), doc.data() + doc.size(), root.get(),
                     &errors)) {
    throw ParseException(errors);
  }
  return formats::json::detail::Value(std::move(root));
}

formats::json::Value FromStream(std::istream& is) {
  if (!is) {
    throw BadStreamException(is);
  }

  std::string res;
  while (is.good()) {
    std::array<char, kBufSize> buf;
    is.read(buf.data(), buf.size());
    res.append(buf.data(), is.gcount());
  }
  return FromString(res);
}

formats::json::Value FromFile(const std::string& path) {
  std::ifstream is(path);
  return FromStream(is);
}

void Serialize(const formats::json::Value& doc, std::ostream& os) {
  Json::StreamWriterBuilder builder;
  builder["commentStyle"] = "None";
  builder["indentation"] = "";

  // TODO: think of using writer pool
  std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
  writer->write(static_cast<const formats::json::detail::Value&>(doc).Get(),
                &os);
  if (!os) {
    throw BadStreamException(os);
  }
}

std::string ToString(const formats::json::Value& doc) {
  std::ostringstream os;
  Serialize(doc, os);
  return os.str();
}

}  // namespace json
}  // namespace formats
