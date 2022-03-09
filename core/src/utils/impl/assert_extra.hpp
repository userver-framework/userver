#pragma once

#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

[[noreturn]] void AbortWithStacktrace(std::string_view message) noexcept;

}  // namespace utils::impl

USERVER_NAMESPACE_END
