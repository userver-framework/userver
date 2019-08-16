#include <formats/json/serialize.hpp>

#include <array>
#include <cstring>
#include <fstream>
#include <memory>
#include <sstream>

#include <json/reader.h>
#include <json/writer.h>

#include <formats/json/exception.hpp>

namespace {
constexpr size_t kBufSize = 32 * 1024ull;
}  // namespace

namespace formats {
namespace json {

formats::json::Value FromString(utils::string_view doc) {
  if (doc.empty()) {
    throw ParseException("JSON document is empty");
  }

  thread_local std::unique_ptr<Json::CharReader> reader([]() {
    Json::CharReaderBuilder builder;
    builder["allowComments"] = false;
    return builder.newCharReader();
  }());
  auto root = std::make_shared<Json::Value>();
  std::string errors;
  if (!reader->parse(doc.data(), doc.data() + doc.size(), root.get(),
                     &errors)) {
    throw ParseException(errors);
  }
  return formats::json::Value(std::move(root));
}

formats::json::Value FromStream(std::istream& is) {
  if (!is) {
    throw BadStreamException(is);
  }

  std::string res;
  while (is.good()) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    std::array<char, kBufSize> buf;
    is.read(buf.data(), buf.size());
    res.append(buf.data(), is.gcount());
  }
  return FromString(res);
}

void Serialize(const formats::json::Value& doc, std::ostream& os) {
  thread_local std::unique_ptr<Json::StreamWriter> writer([]() {
    Json::StreamWriterBuilder builder;
    builder["commentStyle"] = "None";
    builder["indentation"] = "";
    return builder.newStreamWriter();
  }());
  writer->write(doc.GetNative(), &os);
  if (!os) {
    throw BadStreamException(os);
  }
}

std::string ToString(const formats::json::Value& doc) {
  std::ostringstream os;
  Serialize(doc, os);
  return os.str();
}

namespace blocking {

formats::json::Value FromFile(const std::string& path) {
  std::ifstream is(path);
  return FromStream(is);
}

}  // namespace blocking

}  // namespace json
}  // namespace formats
