#pragma once

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

enum class ReopenMode {
  kAppend,
  kTruncate,
};

}  // namespace logging::impl

USERVER_NAMESPACE_END
