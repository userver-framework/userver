#include <gtest/gtest.h>

#include <userver/compression/zstd.hpp>
#include <zstd.h>

USERVER_NAMESPACE_BEGIN

TEST(Zstd, DecompressSmall) {
  const std::size_t kSize = 8;
  const char* kBuf = "abcdefgh";
  std::string str(kBuf);

  const std::size_t max_size = ZSTD_compressBound(kSize);
  std::string comp_buf(max_size, '\0');

  const std::size_t comp_size =
      ZSTD_compress(comp_buf.data(), max_size, kBuf, kSize, 1);

  if (ZSTD_isError(comp_size)) {
    ADD_FAILURE() << "Couldn't compress data!";
    return;
  }

  auto decomp_str = compression::zstd::Decompress(
      std::string_view(comp_buf.data(), comp_size), max_size);

  EXPECT_EQ(str, decomp_str);
}

TEST(Zstd, DecompressLarge) {
  const std::size_t kSize = 16'000;
  std::string str(kSize, 'a');

  const std::size_t max_size = ZSTD_compressBound(kSize);
  std::string comp_buf(max_size, '\0');

  const std::size_t comp_size =
      ZSTD_compress(comp_buf.data(), max_size, str.data(), kSize, 1);

  if (ZSTD_isError(comp_size)) {
    ADD_FAILURE() << "Couldn't compress data!";
    return;
  }

  auto decomp_str = compression::zstd::Decompress(
      std::string_view(comp_buf.data(), comp_size), max_size);

  EXPECT_EQ(str, decomp_str);
}

TEST(Zstd, TestOverflow) {
  const std::string big_msg("This is a \"Very long\" msg!");

  const std::size_t max_size = ZSTD_compressBound(big_msg.size());
  std::string comp_buf(max_size, '\0');

  const std::size_t comp_size =
      ZSTD_compress(comp_buf.data(), max_size, big_msg.data(), big_msg.size(), 1);

  if (ZSTD_isError(comp_size)) {
    ADD_FAILURE() << "Couldn't compress data!";
    return;
  }

  EXPECT_THROW(compression::zstd::Decompress(std::string_view(comp_buf.data(), comp_size), big_msg.size() / 2),
               compression::TooBigError);
}

USERVER_NAMESPACE_END
