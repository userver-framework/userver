#include <userver/compression/zstd.hpp>

#include <memory>

#include <zstd.h>
#include <zstd_errors.h>

USERVER_NAMESPACE_BEGIN

namespace compression::zstd {

namespace {
// The same size as in ZSTD_DStreamOutSize();
const size_t kDecompressBufferSize = ZSTD_DStreamOutSize();
}  // namespace

const int kDefaultCompressionLevel = ZSTD_CLEVEL_DEFAULT;

std::string DecompressStream(std::string_view compressed, size_t max_size) {
    std::string decompressed;
    std::string buf(kDecompressBufferSize, '\0');

    auto* stream = ZSTD_createDStream();
    if (stream == nullptr) {
        throw std::runtime_error("Couldn't create ZSTD decompression stream");
    }
    auto stream_del = [](ZSTD_DStream* ptr) { ZSTD_freeDStream(ptr); };
    auto stream_guard = std::unique_ptr<ZSTD_DStream, decltype(stream_del)>(stream, stream_del);

    {
        const auto err_code = ZSTD_DCtx_reset(stream, ZSTD_ResetDirective::ZSTD_reset_session_and_parameters);
        if (ZSTD_isError(err_code)) {
            throw ErrWithCode(ZSTD_getErrorName(err_code));
        }
    }

    for (size_t cur_pos(0); cur_pos < compressed.size();) {
        ZSTD_inBuffer
            input{compressed.data() + cur_pos, std::min(kDecompressBufferSize, compressed.size() - cur_pos), 0};

        while (input.pos < input.size) {
            ZSTD_outBuffer output{buf.data(), buf.size(), 0};

            if (const auto ret = ZSTD_decompressStream(stream, &output, &input); ZSTD_isError(ret)) {
                throw ErrWithCode(ZSTD_getErrorName(ret));
            }

            // Must check before append: one input chunk may expand far past max_size.
            if (decompressed.size() + output.pos > max_size) {
                throw TooBigError();
            }

            decompressed.append(static_cast<char*>(output.dst), output.pos);
        }

        cur_pos += input.size;
    }

    return decompressed;
}

std::string Decompress(std::string_view compressed, size_t max_size) {
    const auto decompressed_size = ZSTD_getFrameContentSize(compressed.data(), compressed.size());

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
    if (const auto
            ret = ZSTD_decompress(decompressed.data(), decompressed.capacity(), compressed.data(), compressed.size());
        ZSTD_isError(ret))
    {
        throw ErrWithCode(ZSTD_getErrorName(ret));
    }

    return decompressed;
}

std::string Compress(std::string_view data, int compression_level) {
    const auto capacity = ZSTD_compressBound(data.size());
    const auto buffer = std::make_unique_for_overwrite<char[]>(capacity);
    const auto compressed_size = ZSTD_compress(buffer.get(), capacity, data.data(), data.size(), compression_level);
    if (ZSTD_isError(compressed_size)) {
        throw std::runtime_error(ZSTD_getErrorName(compressed_size));
    }
    return std::string(buffer.get(), compressed_size);
}

}  // namespace compression::zstd
USERVER_NAMESPACE_END
