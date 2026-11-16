#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <latch>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <zstd.h>

#include <userver/compression/zstd.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr int kFastCompressionLevel = -2;
constexpr int kSlowCompressionLevel = 10;

// Streaming compression without a pledged size yields ZSTD_CONTENTSIZE_UNKNOWN,
// which forces Decompress() to use the streaming decompression path.
std::string CompressZerosStreamUnknownSize(std::size_t zero_bytes) {
    auto* cctx = ZSTD_createCCtx();
    EXPECT_NE(cctx, nullptr);
    auto cctx_guard = std::unique_ptr<ZSTD_CCtx, decltype(&ZSTD_freeCCtx)>(cctx, &ZSTD_freeCCtx);

    EXPECT_FALSE(ZSTD_isError(ZSTD_CCtx_setParameter(cctx, ZSTD_c_compressionLevel, 1)));

    std::string compressed;
    std::string out_buf(ZSTD_CStreamOutSize(), '\0');
    const std::string zeros(ZSTD_CStreamInSize(), '\0');

    std::size_t remaining = zero_bytes;
    while (remaining > 0) {
        const std::size_t chunk = std::min(remaining, zeros.size());
        ZSTD_inBuffer input{zeros.data(), chunk, 0};
        while (input.pos < input.size) {
            ZSTD_outBuffer output{out_buf.data(), out_buf.size(), 0};
            const auto ret = ZSTD_compressStream2(cctx, &output, &input, ZSTD_e_continue);
            EXPECT_FALSE(ZSTD_isError(ret)) << ZSTD_getErrorName(ret);
            compressed.append(out_buf.data(), output.pos);
        }
        remaining -= chunk;
    }

    ZSTD_inBuffer empty{nullptr, 0, 0};
    for (;;) {
        ZSTD_outBuffer output{out_buf.data(), out_buf.size(), 0};
        const auto ret = ZSTD_compressStream2(cctx, &output, &empty, ZSTD_e_end);
        EXPECT_FALSE(ZSTD_isError(ret)) << ZSTD_getErrorName(ret);
        compressed.append(out_buf.data(), output.pos);
        if (ret == 0) {
            break;
        }
    }

    EXPECT_EQ(ZSTD_getFrameContentSize(compressed.data(), compressed.size()), ZSTD_CONTENTSIZE_UNKNOWN);
    return compressed;
}

std::string MakeSkippableFrame() {
    // RFC 8878, 3.1.2: little-endian magic 0x184D2A50, little-endian content size, content.
    return std::string{"\x50\x2A\x4D\x18\x04\x00\x00\x00skip", 12};
}

}  // namespace

TEST(Zstd, DecompressConcatenatedFrames) {
    const std::string first = "abcde";
    const std::string second = "XYZ";
    const auto compressed = compression::zstd::Compress(first) + compression::zstd::Compress(second);

    const auto decompressed = compression::zstd::Decompress(compressed, first.size() + second.size());

    EXPECT_EQ(decompressed, first + second);
    EXPECT_EQ(decompressed.c_str()[decompressed.size()], '\0');
}

TEST(Zstd, DecompressConcatenatedFramesTooBig) {
    const std::string first = "abcde";
    const std::string second = "XYZ";
    const auto compressed = compression::zstd::Compress(first) + compression::zstd::Compress(second);

    EXPECT_THROW(compression::zstd::Decompress(compressed, first.size()), compression::TooBigError);
}

TEST(Zstd, DecompressSkippableFrameFirst) {
    const std::string original = "helloworld";
    const auto compressed = MakeSkippableFrame() + compression::zstd::Compress(original);

    EXPECT_EQ(compression::zstd::Decompress(compressed, original.size()), original);
}

TEST(Zstd, DecompressLargeWindowFrameWithSkippableSuffix) {
    using namespace std::string_literals;
    const auto frame =
        "\x28\xb5\x2f\xfd"          // Zstandard magic.
        "\x80\x90\x01\x00\x02\x00"  // 256 MiB window, 131073 bytes of content.
        "\x02\x00\x10\x61"          // RLE block of 131072 'a' bytes.
        "\x0b\x00\x00\x61"s;        // Final RLE block of one 'a'.
    const std::string expected(131073, 'a');

    EXPECT_EQ(compression::zstd::Decompress(frame + MakeSkippableFrame(), expected.size()), expected);
}

TEST(Zstd, DecompressTruncatedFrame) {
    const auto compressed = compression::zstd::Compress("helloworld");

    EXPECT_THROW(
        compression::zstd::Decompress(std::string_view{compressed}.substr(0, compressed.size() - 1), 10),
        compression::ErrWithCode
    );
}

TEST(Zstd, DecompressTruncatedSecondFrame) {
    const std::string first = "abcde";
    const std::string second = "XYZ";
    auto compressed = compression::zstd::Compress(first) + compression::zstd::Compress(second);
    compressed.pop_back();

    EXPECT_THROW(compression::zstd::Decompress(compressed, first.size() + second.size()), compression::ErrWithCode);
}

TEST(Zstd, DecompressStreamMultipleOutputBuffers) {
    const std::string prefix = "prefix";
    const std::string zeros(3 * ZSTD_DStreamOutSize() + 1, '\0');
    const auto compressed = CompressZerosStreamUnknownSize(zeros.size());

    EXPECT_EQ(compression::zstd::Decompress(compressed, zeros.size()), zeros);
    EXPECT_EQ(
        compression::zstd::Decompress(compression::zstd::Compress(prefix) + compressed, prefix.size() + zeros.size()),
        prefix + zeros
    );
}

TEST(Zstd, RejectTruncatedUnknownSizeFrame) {
    constexpr std::size_t kOriginalSize = 1024;
    const auto compressed = CompressZerosStreamUnknownSize(kOriginalSize);
    ASSERT_EQ(compression::zstd::Decompress(compressed, kOriginalSize), std::string(kOriginalSize, '\0'));

    for (std::size_t size = 0; size < compressed.size(); ++size) {
        EXPECT_THROW(
            compression::zstd::Decompress(std::string_view{compressed}.substr(0, size), kOriginalSize),
            compression::ErrWithCode
        ) << size;
    }
}

TEST(Zstd, RejectMissingUnknownSizeChecksum) {
    auto* cctx = ZSTD_createCCtx();
    ASSERT_NE(cctx, nullptr);
    const std::unique_ptr<ZSTD_CCtx, decltype(&ZSTD_freeCCtx)> guard{cctx, &ZSTD_freeCCtx};
    ASSERT_FALSE(ZSTD_isError(ZSTD_CCtx_setParameter(cctx, ZSTD_c_contentSizeFlag, 0)));
    ASSERT_FALSE(ZSTD_isError(ZSTD_CCtx_setParameter(cctx, ZSTD_c_checksumFlag, 1)));
    const std::string original = "hello world";
    std::string compressed(ZSTD_compressBound(original.size()), '\0');
    const auto size = ZSTD_compress2(cctx, compressed.data(), compressed.size(), original.data(), original.size());
    ASSERT_FALSE(ZSTD_isError(size));
    compressed.resize(size);
    ASSERT_EQ(ZSTD_getFrameContentSize(compressed.data(), compressed.size()), ZSTD_CONTENTSIZE_UNKNOWN);
    ASSERT_EQ(compression::zstd::Decompress(compressed, original.size()), original);

    constexpr std::size_t kChecksumSize = 4;
    compressed.resize(compressed.size() - kChecksumSize);

    EXPECT_THROW(compression::zstd::Decompress(compressed, original.size()), compression::ErrWithCode);
}

TEST(Zstd, CompressEmpty) {
    const std::string original;

    const auto compressed = compression::zstd::Compress(original);
    const auto decompressed = compression::zstd::Decompress(compressed, 1);

    EXPECT_EQ(compressed, compression::zstd::Compress(original, compression::zstd::kDefaultCompressionLevel));
    EXPECT_EQ(decompressed, original);
}

TEST(Zstd, CompressLargeRepetitive) {
    const std::string original(1024 * 1024, 'a');

    const auto fast = compression::zstd::Compress(original, kFastCompressionLevel);
    const auto slow = compression::zstd::Compress(original, kSlowCompressionLevel);

    EXPECT_EQ(compression::zstd::Decompress(fast, original.size()), original);
    EXPECT_EQ(compression::zstd::Decompress(slow, original.size()), original);
    EXPECT_NE(fast, slow);
    EXPECT_LT(slow.size(), original.size());
}

TEST(Zstd, DecompressSmall) {
    const std::string str("abcdefgh");

    std::string comp_buf(ZSTD_compressBound(str.size()), '\0');

    const auto comp_size = ZSTD_compress(comp_buf.data(), comp_buf.capacity(), str.data(), str.size(), 1);

    if (ZSTD_isError(comp_size)) {
        ADD_FAILURE() << "Couldn't compress data!";
        return;
    }

    auto decompressed = compression::zstd::Decompress(std::string_view(comp_buf.data(), comp_size), str.size());

    EXPECT_EQ(str, decompressed);
}

TEST(Zstd, DecompressLarge) {
    constexpr std::size_t kSize = 16'000;
    const std::string str(kSize, 'a');

    const auto max_size = ZSTD_compressBound(kSize);
    std::string comp_buf(max_size, '\0');

    const auto comp_size = ZSTD_compress(comp_buf.data(), comp_buf.capacity(), str.data(), kSize, 1);

    if (ZSTD_isError(comp_size)) {
        ADD_FAILURE() << "Couldn't compress data!";
        return;
    }

    auto decompressed = compression::zstd::Decompress(std::string_view(comp_buf.data(), comp_size), str.size());

    EXPECT_EQ(str, decompressed);
}

TEST(Zstd, TestOverflow) {
    const std::string big_msg("This is a \"Very long\" msg!");

    const auto max_size = ZSTD_compressBound(big_msg.size());
    std::string comp_buf(max_size, '\0');

    const auto comp_size = ZSTD_compress(comp_buf.data(), comp_buf.capacity(), big_msg.data(), big_msg.size(), 1);

    if (ZSTD_isError(comp_size)) {
        ADD_FAILURE() << "Couldn't compress data!";
        return;
    }

    EXPECT_THROW(
        compression::zstd::Decompress(std::string_view(comp_buf.data(), comp_size), big_msg.size() / 2),
        compression::TooBigError
    );
}

// Compression bomb with unknown frame content size: DecompressStream must honour
// max_size. Without a per-output-chunk check a nearly-ZSTD_DStreamOutSize input
// chunk expands far past max_size (OOM) before TooBigError.
TEST(Zstd, DecompressStreamOverflow) {
    static const std::size_t kCompressedSize = ZSTD_DStreamOutSize();
    // 4095 MiB of zeros compresses to just below ZSTD_DStreamOutSize.
    constexpr std::size_t kZeroBytes = 4095ull * 1024 * 1024;

    const auto compressed = CompressZerosStreamUnknownSize(kZeroBytes);
    ASSERT_LT(compressed.size(), kCompressedSize);
    ASSERT_GT(compressed.size(), kCompressedSize - 64);

    constexpr int kThreads = 128;
    std::atomic<int> too_big_count{0};
    std::latch start{kThreads};
    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&] {
            start.arrive_and_wait();
            try {
                compression::zstd::Decompress(compressed, kCompressedSize);
            } catch (const compression::TooBigError&) {
                too_big_count.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }
    EXPECT_EQ(too_big_count.load(), kThreads);
}

USERVER_NAMESPACE_END
