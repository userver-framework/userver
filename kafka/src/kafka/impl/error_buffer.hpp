#pragma once

#include <array>
#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

/// @brief Default error buffer size used in `librdkafka` examples
/// @see
/// https://github.com/search?q=repo%3Aconfluentinc%2Flibrdkafka%20errstr%5B512%5D&type=code
constexpr std::size_t kErrorBufferSize = 512;

/// @brief Error buffer of `kErrorBufferSize` into which `librdkafka` prints
/// configuration errors
using ErrorBuffer = std::array<char, kErrorBufferSize>;

[[noreturn]] void PrintErrorAndThrow(std::string_view failed_action, const ErrorBuffer& err_buf);

}  // namespace kafka::impl

USERVER_NAMESPACE_END
