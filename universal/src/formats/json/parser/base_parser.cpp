#include <userver/formats/json/parser/base_parser.hpp>

#include <userver/compiler/impl/nodebug.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json::parser {

// vtable anchor functions
USERVER_IMPL_NODEBUG BaseParser::~BaseParser() = default;

USERVER_IMPL_NODEBUG void BaseParser::Null() { Throw("null"); }
USERVER_IMPL_NODEBUG void BaseParser::Bool(bool) { Throw("bool"); }
USERVER_IMPL_NODEBUG void BaseParser::Int64(int64_t) { Throw("integer"); }
USERVER_IMPL_NODEBUG void BaseParser::Uint64(uint64_t) { Throw("integer"); }
USERVER_IMPL_NODEBUG void BaseParser::Double(double) { Throw("double"); }
USERVER_IMPL_NODEBUG void BaseParser::String(std::string_view) { Throw("string"); }
USERVER_IMPL_NODEBUG void BaseParser::StartObject() { Throw("object"); }
USERVER_IMPL_NODEBUG void BaseParser::Key(std::string_view key) { Throw("field '" + std::string(key) + "'"); }
USERVER_IMPL_NODEBUG void BaseParser::EndObject() { Throw("'}'"); }
USERVER_IMPL_NODEBUG void BaseParser::StartArray() { Throw("array"); }
USERVER_IMPL_NODEBUG void BaseParser::EndArray() { Throw("']'"); }

void BaseParser::Throw(const std::string& found) {
    throw InternalParseError(Expected() + " was expected, but " + found + " found");
}

}  // namespace formats::json::parser

USERVER_NAMESPACE_END
