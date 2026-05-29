#include <userver/utils/invariant_error.hpp>

#include <userver/compiler/impl/nodebug.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

// vtable anchor functions
USERVER_IMPL_NODEBUG InvariantError::~InvariantError() = default;

}  // namespace utils

USERVER_NAMESPACE_END
