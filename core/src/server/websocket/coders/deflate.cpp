#include "deflate.h"
#include <zlib.h>

namespace userver::websocket {

std::optional<std::vector<std::byte>> deflate(
    const utils::impl::Span<const std::byte>& in) noexcept {
  z_stream z;

  z.next_in = Z_NULL;
  z.zalloc = Z_NULL;
  z.zfree = Z_NULL;
  z.opaque = Z_NULL;

  if (Z_OK != deflateInit2(&z, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 8,
                           Z_DEFAULT_STRATEGY)) {
    return {};
  }

  z.next_in = (Bytef*)(in.data());
  z.avail_in = in.size();
  z.total_in = 0;

  std::vector<std::byte> result;
  result.resize(compressBound(in.size()));

  z.next_out = (Bytef*)result.data();
  z.avail_out = result.size();

  if (Z_STREAM_END != deflate(&z, Z_FINISH)) {
    deflateEnd(&z);
    return {};
  }

  if (Z_OK != deflateEnd(&z)) return {};

  result.resize(z.total_out);
  return result;
}

}  // namespace userver::websocket
