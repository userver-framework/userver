#include <storages/postgres/io/string_types.hpp>

namespace storages::postgres::io {

//@{
/** @name Register parsers for additional PG types mapped to string */
template <>
struct PgToCpp<PredefinedOids::kChar, char>
    : detail::PgToCppPredefined<PredefinedOids::kChar, char> {};
template <>
struct PgToCpp<PredefinedOids::kBpchar, std::string>
    : detail::PgToCppPredefined<PredefinedOids::kBpchar, std::string> {};
template <>
struct PgToCpp<PredefinedOids::kVarchar, std::string>
    : detail::PgToCppPredefined<PredefinedOids::kVarchar, std::string> {};
//@}

namespace {

const bool kReference = detail::ForceReference(
    PgToCpp<PredefinedOids::kChar, char>::init_,
    PgToCpp<PredefinedOids::kBpchar, std::string>::init_,
    PgToCpp<PredefinedOids::kVarchar, std::string>::init_);

}  // namespace

void BufferParser<std::string, DataFormat::kTextDataFormat>::operator()(
    const FieldBuffer& buffer) {
  std::string{buffer.buffer, buffer.length}.swap(value);
}

void BufferParser<std::string, DataFormat::kBinaryDataFormat>::operator()(
    const FieldBuffer& buffer) {
  std::string{buffer.buffer, buffer.length}.swap(value);
}

}  // namespace storages::postgres::io
