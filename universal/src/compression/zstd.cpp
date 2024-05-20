#include <userver/compression/zstd.hpp>

#include <zstd.h>
#include <zstd_errors.h>

USERVER_NAMESPACE_BEGIN

namespace compression::zstd {

namespace {
// The same size as in ZSTD_DStreamOutSize();
const size_t kDecompressBufferSize = ZSTD_DStreamOutSize();
}  // namespace

std::string Decompress(std::string_view compressed, size_t max_size) {
  std::string decompressed;
  std::string buf(kDecompressBufferSize, ' ');

  auto* stream = ZSTD_createDStream();
  if (stream == nullptr) {
    throw std::runtime_error("Couldn't create ZSTD decompression stream");
  }

  {
    ZSTD_ResetDirective reset = ZSTD_ResetDirective::ZSTD_reset_session_and_parameters;
    auto err_code = ZSTD_DCtx_reset(stream, reset);
    if (ZSTD_isError(err_code)) {
      throw ErrWithCode(ZSTD_getErrorName(err_code));
    }
  }


  auto stream_del = [](ZSTD_DStream* ptr) { ZSTD_freeDStream(ptr); };
  auto stream_guard =
      std::unique_ptr<ZSTD_DStream, decltype(stream_del)>(stream, stream_del);

  for (size_t cur_pos(0); cur_pos < compressed.size();) {
    ZSTD_inBuffer input{
        compressed.data() + cur_pos,
        std::min(kDecompressBufferSize, compressed.size() - cur_pos), 0};
    ZSTD_outBuffer output{buf.data(), buf.size(), 0};
    auto* output_pos = static_cast<char*>(output.dst);

    while (input.pos < input.size) {
      const auto ret = ZSTD_decompressStream(stream, &output, &input);

      if (ZSTD_isError(ret)) {
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

  return decompressed;
}

}  // namespace compression::zstd

USERVER_NAMESPACE_END
