#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <latch>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <zstd.h>

#include <userver/compression/zstd.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

// Streaming compression without a pledged size yields ZSTD_CONTENTSIZE_UNKNOWN,
// which forces Decompress() to use DecompressStream().
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

}  // namespace

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
