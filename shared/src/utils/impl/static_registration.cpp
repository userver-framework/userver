#include <userver/utils/impl/static_registration.hpp>

#include <fmt/format.h>

#include <userver/utils/threads.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

std::atomic<bool> is_static_registration_finished{false};

void AssertStaticRegistrationAllowed(std::string_view operation_name) {
  UINVARIANT(!is_static_registration_finished && utils::IsMainThread(),
             fmt::format("{} is only allowed at static initialization time",
                         operation_name));
}

void FinishStaticRegistration() noexcept {
  is_static_registration_finished = true;
}

}  // namespace utils::impl

USERVER_NAMESPACE_END
