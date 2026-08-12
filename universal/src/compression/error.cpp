#include <userver/compression/error.hpp>

#include <userver/compiler/impl/nodebug.hpp>

USERVER_NAMESPACE_BEGIN

namespace compression {

// vtable anchor functions
USERVER_IMPL_NODEBUG DecompressionError::~DecompressionError() = default;
USERVER_IMPL_NODEBUG TooBigError::~TooBigError() = default;
USERVER_IMPL_NODEBUG ErrWithCode::~ErrWithCode() = default;

}  // namespace compression
USERVER_NAMESPACE_END
