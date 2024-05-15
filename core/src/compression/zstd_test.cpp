#include <gtest/gtest.h>

#include <userver/logging/log.hpp>
#include <compression/zstd.hpp>
#include <zstd.h>

USERVER_NAMESPACE_BEGIN

TEST(Zstd, DecompressSmall) {
  const std::size_t kSize = 8;
  const char* kBuf = "abcdefg"; // 8 bytes
  std::string str(kBuf);

  const std::size_t max_size = ZSTD_compressBound(kSize);
  char* comp_buf = new char (max_size);

  const std::size_t comp_size = ZSTD_compress(comp_buf, max_size, kBuf, kSize, 1);

  if (ZSTD_isError(comp_size)) {
    ADD_FAILURE() << "Couldn't compress data!";
    return;
  }

  auto decomp_str = compression::zstd::Decompress(std::string_view(kBuf), kSize);

  EXPECT_EQ(str, decomp_str);
  }

USERVER_NAMESPACE_END
