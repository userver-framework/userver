#include <userver/compression/zstd.hpp>

#include <zstd.h>
#include <zstd_errors.h>

USERVER_NAMESPACE_BEGIN

namespace compression::zstd {

namespace {
// The same size as in ZSTD_DStreamOutSize();
const size_t kDecompressBufferSize = ZSTD_DStreamOutSize();
}  // namespace

std::string DecompressStream(std::string_view compressed, size_t max_size) {
  std::string decompressed;
  std::string buf(kDecompressBufferSize, '\0');

  auto* stream = ZSTD_createDStream();
  if (stream == nullptr) {
    throw std::runtime_error("Couldn't create ZSTD decompression stream");
  }
  auto stream_del = [](ZSTD_DStream* ptr) { ZSTD_freeDStream(ptr); };
  auto stream_guard =
      std::unique_ptr<ZSTD_DStream, decltype(stream_del)>(stream, stream_del);

  {
    const auto err_code = ZSTD_DCtx_reset(
        stream, ZSTD_ResetDirective::ZSTD_reset_session_and_parameters);
    if (ZSTD_isError(err_code)) {
      throw ErrWithCode(ZSTD_getErrorName(err_code));
    }
  }

  for (size_t cur_pos(0); cur_pos < compressed.size();) {
    ZSTD_inBuffer input{
        compressed.data() + cur_pos,
        std::min(kDecompressBufferSize, compressed.size() - cur_pos), 0};

    while (input.pos < input.size) {
      ZSTD_outBuffer output{buf.data(), buf.size(), 0};

      const auto ret = ZSTD_decompressStream(stream, &output, &input);
      if (ZSTD_isError(ret)) {
        throw ErrWithCode(ZSTD_getErrorName(ret));
      }

      decompressed.append(static_cast<char*>(output.dst), output.size);
    }

    if (decompressed.size() > max_size) {
      throw TooBigError();
    }

    cur_pos += input.size;
  }

  return decompressed;
}

std::string Decompress(std::string_view compressed, size_t max_size) {
  auto decompressed_size =
      ZSTD_getFrameContentSize(compressed.data(), compressed.size());

  switch (decompressed_size) {
    case ZSTD_CONTENTSIZE_UNKNOWN:
      return DecompressStream(compressed, max_size);
    case ZSTD_CONTENTSIZE_ERROR:
      throw std::runtime_error("Error while getting size");
    default:
      if (decompressed_size > max_size) {
        throw TooBigError();
      }
  }

  std::string decompressed(decompressed_size, '\0');
  auto ret = ZSTD_decompress(decompressed.data(), decompressed.capacity(),
                             compressed.data(), compressed.size());

  if (ZSTD_isError(ret)) {
    throw ErrWithCode(ZSTD_getErrorName(ret));
  }

  return decompressed;
}

}  // namespace compression::zstd
USERVER_NAMESPACE_END
