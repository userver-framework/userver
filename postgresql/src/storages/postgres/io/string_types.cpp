#include <userver/storages/postgres/io/string_types.hpp>

USERVER_NAMESPACE_BEGIN

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
template <>
struct PgToCpp<PredefinedOids::kName, std::string>
    : detail::PgToCppPredefined<PredefinedOids::kName, std::string> {};
//@}

namespace {

const bool kReference = detail::ForceReference(
    CppToPg<char>::init_, CppToPg<std::string>::init_,
    PgToCpp<PredefinedOids::kChar, char>::init_,
    PgToCpp<PredefinedOids::kBpchar, std::string>::init_,
    PgToCpp<PredefinedOids::kVarchar, std::string>::init_,
    PgToCpp<PredefinedOids::kName, std::string>::init_);

}  // namespace

void BufferParser<std::string>::operator()(const FieldBuffer& buffer) {
  buffer.ToString().swap(value);
}

}  // namespace storages::postgres::io

USERVER_NAMESPACE_END
