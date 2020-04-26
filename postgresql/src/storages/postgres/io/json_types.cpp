#include <storages/postgres/io/json_types.hpp>

#include <string_view>

namespace storages::postgres::io {

template <>
struct PgToCpp<PredefinedOids::kJson, formats::json::Value>
    : detail::PgToCppPredefined<PredefinedOids::kJson, formats::json::Value> {};

namespace {

const bool kReference = detail::ForceReference(
    CppToPg<formats::json::Value>::init_,
    PgToCpp<PredefinedOids::kJson, formats::json::Value>::init_);

}  // namespace

namespace detail {

void JsonParser::operator()(const FieldBuffer& buffer) {
  if (buffer.length == 0) {
    throw InvalidInputBufferSize{0, "for a json type"};
  }
  const char* start = reinterpret_cast<const char*>(buffer.buffer);
  auto length = buffer.length;
  if (*start == 1) {  // this is jsonb
    ++start;
    --length;
  }

  std::string_view json_string(start, length);
  value = formats::json::FromString(json_string);
}

}  // namespace detail

}  // namespace storages::postgres::io
