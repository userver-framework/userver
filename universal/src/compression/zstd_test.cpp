#include <gtest/gtest.h>

#include <zstd.h>
#include <userver/compression/zstd.hpp>

USERVER_NAMESPACE_BEGIN

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

USERVER_NAMESPACE_END
