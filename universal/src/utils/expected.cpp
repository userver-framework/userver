#include <userver/utils/expected.hpp>

#include <userver/compiler/impl/nodebug.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

// vtable anchor functions
USERVER_IMPL_NODEBUG const char* bad_expected_access::what() const noexcept { return message_.c_str(); }

}  // namespace utils

USERVER_NAMESPACE_END
