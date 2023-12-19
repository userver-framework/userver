#pragma once

#include <atomic>
#include <string_view>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

extern std::atomic<bool> is_static_registration_finished;

void AssertStaticRegistrationAllowed(std::string_view operation_name);

// Defined inline to optimize away in Release builds
inline void AssertStaticRegistrationFinished() noexcept {
  UASSERT_MSG(is_static_registration_finished,
              "This operation is only allowed after static initialization "
              "completes, see stacktrace");
}

// Called in TaskProcessor constructor, before the creation of first
// TaskContext. Ideally should be called in the beginning of main(). As long as
// AssertStaticRegistrationFinished does not fire, it's placed in enough places.
void FinishStaticRegistration() noexcept;

}  // namespace utils::impl

USERVER_NAMESPACE_END
