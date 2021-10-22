#include <userver/utils/any_movable.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

bool AnyMovable::HasValue() const noexcept { return content_ != nullptr; }

void AnyMovable::Reset() noexcept { content_.reset(); }

void AnyMovable::HolderDeleter::operator()(
    AnyMovable::HolderBase* holder) noexcept {
  UASSERT(holder->deleter);
  holder->deleter(*holder);
}

const char* BadAnyMovableCast::what() const noexcept {
  return "utils::bad_any_movable_cast: "
         "failed conversion using utils::AnyMovableCast";
}

}  // namespace utils

USERVER_NAMESPACE_END
