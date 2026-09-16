#include <userver/storages/postgres/io/json_types.hpp>

#include <string>
#include <string_view>

#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/stream.hpp>

#include <userver/formats/json/serialize.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::io {

template <>
struct PgToCpp<PredefinedOids::kJson, formats::json::Value>
    : detail::PgToCppPredefined<PredefinedOids::kJson, formats::json::Value> {};

template <>
struct PgToCpp<PredefinedOids::kJsonb, formats::json::RawString>
    : detail::PgToCppPredefined<PredefinedOids::kJsonb, formats::json::RawString> {};

template <>
struct PgToCpp<PredefinedOids::kJson, formats::json::RawString>
    : detail::PgToCppPredefined<PredefinedOids::kJson, formats::json::RawString> {};

namespace {

const bool kReferenceJsonValue = detail::ForceReference(
    CppToPg<formats::json::Value>::init,
    PgToCpp<PredefinedOids::kJson, formats::json::Value>::init
);

const bool kReferenceJsonRawString = detail::ForceReference(
    CppToPg<formats::json::RawString>::init,
    PgToCpp<PredefinedOids::kJsonb, formats::json::RawString>::init,
    PgToCpp<PredefinedOids::kJson, formats::json::RawString>::init
);

}  // namespace

namespace detail {

std::string_view ExtractJsonPayload(const FieldBuffer& buffer) {
    if (buffer.length == 0) {
        throw InvalidInputBufferSize{"Invalid buffer size 0 for a json type"};
    }
    const char* start = reinterpret_cast<const char*>(buffer.buffer);
    auto length = buffer.length;
    if (*start == kJsonbVersion) {
        ++start;
        --length;
    }

    return {start, length};
}

void JsonRawStringParser::operator()(const FieldBuffer& buffer) {
    value = formats::json::RawString{std::string{ExtractJsonPayload(buffer)}};
}

void JsonParser::operator()(const FieldBuffer& buffer) {
    value = formats::json::FromString(ExtractJsonPayload(buffer));
}

void JsonValueToBuffer(const formats::json::Value& value, std::vector<char>& buffer) {
    auto sink = boost::iostreams::back_inserter(buffer);
    boost::iostreams::stream os{sink};
    formats::json::Serialize(value, os);
}

void JsonValueToBuffer(const formats::json::Value& value, std::string& buffer) {
    auto sink = boost::iostreams::back_inserter(buffer);
    boost::iostreams::stream os{sink};
    formats::json::Serialize(value, os);
}

}  // namespace detail

}  // namespace storages::postgres::io

USERVER_NAMESPACE_END
