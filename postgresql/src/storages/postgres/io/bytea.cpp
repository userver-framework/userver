#include <userver/storages/postgres/io/bytea.hpp>
#include <userver/storages/postgres/io/string_types.hpp>
#include <userver/storages/postgres/io/type_mapping.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::io {

template <>
struct PgToCpp<PredefinedOids::kBytea, std::string>
    : detail::PgToCppPredefined<PredefinedOids::kBytea, std::string> {};

namespace {

const bool kReference =
    detail::ForceReference(PgToCpp<PredefinedOids::kBytea, std::string>::init_);

}  // namespace

}  // namespace storages::postgres::io

USERVER_NAMESPACE_END
