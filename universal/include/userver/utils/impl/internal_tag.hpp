#pragma once

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

// Guard tag for functions intended only for use only within userver itself.
//
// Consequences of using this tag outside of userver:
// 1. support for your service will be dropped by userver team;
// 2. calling the function outside of userver may break your code up to UB;
// 3. compilation of your service may break after a patch-level userver update.
struct InternalTag final {
  constexpr explicit InternalTag() noexcept = default;
};

}  // namespace utils::impl

USERVER_NAMESPACE_END
