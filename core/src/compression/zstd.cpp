#include <compression/zstd.hpp>

#include <zstd.h>
#include <zstd_errors.h>

USERVER_NAMESPACE_BEGIN

namespace compression::zstd {

namespace {
constexpr size_t kDecompressBufferSize = 1024;
}

std::string Decompress(std::string_view compressed, size_t max_size) {
  std::string decompressed;

  auto* stream = ZSTD_createDStream();
  ZSTD_initDStream(stream);

  size_t cur_pos(0);
  while (cur_pos < compressed.size()) {
    char buf[kDecompressBufferSize];
    ZSTD_inBuffer_s input{
        compressed.data() + cur_pos,
        std::min(kDecompressBufferSize, compressed.size() - cur_pos), 0};

    while (input.pos < input.size) {
      ZSTD_outBuffer output = {buf, sizeof(buf), 0};
      const auto ret = ZSTD_decompressStream(stream, &output, &input);

      if (ZSTD_isError(ret)) {
        ZSTD_freeDStream(stream);
        throw ErrWithCode(ret);
      }

      decompressed.append(static_cast<char*>(output.dst), output.size);
    }

    cur_pos += input.size;
  }

  ZSTD_freeDStream(stream);

  return decompressed;
}

}  // namespace compression::zstd

USERVER_NAMESPACE_END
