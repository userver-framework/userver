#include <userver/compression/zstd.hpp>

#include <memory>

#include <zstd.h>

USERVER_NAMESPACE_BEGIN

namespace compression::zstd {

const int kDefaultCompressionLevel = ZSTD_CLEVEL_DEFAULT;

namespace {

size_t CheckZstdResult(size_t result) {
    if (ZSTD_isError(result)) {
        throw ErrWithCode(ZSTD_getErrorName(result));
    }
    return result;
}

}  // namespace

std::string Decompress(std::string_view compressed, size_t max_size) {
    std::string decompressed;
    const std::unique_ptr<ZSTD_DCtx, decltype(&ZSTD_freeDCtx)> stream{ZSTD_createDCtx(), &ZSTD_freeDCtx};
    if (!stream) {
        throw std::runtime_error("Couldn't create ZSTD decompression stream");
    }

    std::string buf;
    do {
        const auto frame_size = CheckZstdResult(ZSTD_findFrameCompressedSize(compressed.data(), compressed.size()));
        const auto frame = compressed.substr(0, frame_size);
        compressed.remove_prefix(frame_size);

        const auto size = ZSTD_getFrameContentSize(frame.data(), frame.size());
        if (size == ZSTD_CONTENTSIZE_ERROR) {
            throw std::runtime_error("Error while getting size");
        }
        if (size != ZSTD_CONTENTSIZE_UNKNOWN) {
            const auto offset = decompressed.size();
            if (size > max_size - offset) {
                throw TooBigError();
            }
            decompressed.resize(offset + size);
            CheckZstdResult(
                ZSTD_decompressDCtx(stream.get(), decompressed.data() + offset, size, frame.data(), frame.size())
            );
            continue;
        }

        buf.resize(ZSTD_DStreamOutSize());
        ZSTD_inBuffer input{frame.data(), frame.size(), 0};
        while (input.pos < input.size) {
            ZSTD_outBuffer output{buf.data(), buf.size(), 0};
            CheckZstdResult(ZSTD_decompressStream(stream.get(), &output, &input));
            if (output.pos > max_size - decompressed.size()) {
                throw TooBigError();
            }
            decompressed.append(buf.data(), output.pos);
        }
    } while (!compressed.empty());

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
