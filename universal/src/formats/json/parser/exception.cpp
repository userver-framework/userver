#include <userver/formats/json/parser/exception.hpp>

#include <userver/compiler/impl/nodebug.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json::parser {

// vtable anchor functions
USERVER_IMPL_NODEBUG BaseError::~BaseError() = default;
USERVER_IMPL_NODEBUG ParseError::~ParseError() = default;
USERVER_IMPL_NODEBUG InternalParseError::~InternalParseError() = default;

}  // namespace formats::json::parser

USERVER_NAMESPACE_END
