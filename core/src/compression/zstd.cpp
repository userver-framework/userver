#include <compression/zstd.hpp>

#include <zstd.h>
#include <zstd_errors.h>

#include <iostream>

USERVER_NAMESPACE_BEGIN

namespace compression::zstd {

namespace {
  constexpr size_t kDecompressBufferSize = 131'072;
}

std::string Decompress(std::string_view compressed, size_t max_size) {
    std::string decompressed;

  auto* stream = ZSTD_createDStream();

  if (stream == nullptr) {
    throw "Couldn't create ZSTD decompression stream";
  }

  ZSTD_initDStream(stream);

  for (size_t cur_pos(0); cur_pos < compressed.size();) {
    char buf[kDecompressBufferSize];
    ZSTD_inBuffer input{
        compressed.data() + cur_pos,
        std::min(kDecompressBufferSize, compressed.size() - cur_pos), 0};
    ZSTD_outBuffer output{buf, sizeof(buf), 0};
    auto* output_pos = static_cast<char*>(output.dst);

    while (input.pos < input.size) {
      const auto ret = ZSTD_decompressStream(stream, &output, &input);

      if (ZSTD_isError(ret)) {
        ZSTD_freeDStream(stream);
        throw ErrWithCode(ZSTD_getErrorName(ret));
      }

      decompressed.append(output_pos, output_pos + output.pos);
      output_pos = static_cast<char*>(output.dst) + output.pos;
    }

    if (decompressed.size() > max_size) {
      throw TooBigError();
    }

    cur_pos += input.size;
  }

  ZSTD_freeDStream(stream);

  return decompressed;
}

}  // namespace compression::zstd

USERVER_NAMESPACE_END
