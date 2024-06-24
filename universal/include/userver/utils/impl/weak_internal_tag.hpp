#pragma once

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

// Guard tag for functions intended only for internal use.
//
// This version of the tag is weak compared to utils::InternalTag, because it's
// does not formally prevent the user from using the protected functionality.
// This is mainly needed in templates.
//
// Hopefully, utils::impl::WeakInternalTag{} will scare off users that care
// about their code not being broken after a userver update.
struct WeakInternalTag {
  explicit WeakInternalTag() = default;
};

}  // namespace utils::impl

USERVER_NAMESPACE_END
