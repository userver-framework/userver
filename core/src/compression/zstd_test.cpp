#include <userver/utest/utest.hpp>

#include <userver/logging/log.hpp>
#include <compression/zstd.hpp>
#include <zstd.h>

USERVER_NAMESPACE_BEGIN

UTEST(Zstd, DecompressSmall) {
  const std::size_t kSize = 8;
  const char* kBuf = "abcdefgh";
  std::string str(kBuf);

  const std::size_t max_size = ZSTD_compressBound(kSize);
  std::string comp_buf (max_size, '\0');

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

UTEST(Zstd, DecompressLarge) {
  const std::size_t kSize = 16'000;
  std::string str(kSize, 'a');

  const std::size_t max_size = ZSTD_compressBound(kSize);
  std::string comp_buf (max_size, '\0');

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

USERVER_NAMESPACE_END
