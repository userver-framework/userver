#include <userver/utils/checked_pointer.hpp>

#include <stdexcept>

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

[[noreturn]] void ThrowEmptyCheckedPointerException() { throw std::runtime_error{"Empty checked_pointer"}; }

}  // namespace utils::impl

USERVER_NAMESPACE_END
