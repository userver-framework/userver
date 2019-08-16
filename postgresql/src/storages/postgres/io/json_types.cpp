#include <storages/postgres/io/json_types.hpp>

#include <utils/string_view.hpp>

namespace storages::postgres::io {

template <>
struct PgToCpp<PredefinedOids::kJsonb, formats::json::Value>
    : detail::PgToCppPredefined<PredefinedOids::kJsonb, formats::json::Value> {
};

namespace {

const bool kReference = detail::ForceReference(
    CppToPg<formats::json::Value>::init_,
    PgToCpp<PredefinedOids::kJsonb, formats::json::Value>::init_);

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

  ::utils::string_view json_string(start, length);
  value = formats::json::FromString(json_string);
}

}  // namespace detail

}  // namespace storages::postgres::io
