#include "content_encoder.h"
#include <array>
#include <string>
#include "coders/deflate.h"

namespace userver::websocket {

using EncodeFunc = std::optional<std::vector<std::byte>> (*)(
    const utils::impl::Span<const std::byte>&) noexcept;

static std::pair<const char*, EncodeFunc> supported_coders[] = {
    {"deflate", deflate}};

EncodeResult encode(const utils::impl::Span<const std::byte>& in,
                    const std::string& accept_encoding) noexcept {
  for (auto [coderName, coderFunc] : supported_coders) {
    if (accept_encoding.find(coderName) != std::string::npos)
      if (auto coderResult = coderFunc(in); coderResult.has_value())
        return {coderName, coderResult.value()};
  }
  return {};
}

}  // namespace userver::websocket
