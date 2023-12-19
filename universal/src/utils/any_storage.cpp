#include <userver/utils/any_storage.hpp>

#include <userver/utils/impl/static_registration.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::any_storage::impl {

void AssertStaticRegistrationAllowed() {
  utils::impl::AssertStaticRegistrationAllowed("AnyStorageTag creation");
}

}  // namespace utils::any_storage::impl

USERVER_NAMESPACE_END
