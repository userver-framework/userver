#include <userver/compiler/select.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

TEST(Platform, Basic) {
  constexpr std::size_t kStandardLibrary = compiler::SelectSize()
                                               .For64Bit(1)
                                               .For32Bit(2)
                                               .ForLibCpp64(8)
                                               .ForLibStdCpp64(8)
                                               .ForLibCpp32(4)
                                               .ForLibStdCpp32(4);
  EXPECT_EQ(kStandardLibrary, sizeof(void*));

  constexpr std::size_t kBitsChoice =
      compiler::SelectSize().For64Bit(8).For32Bit(4);
  EXPECT_EQ(kBitsChoice, sizeof(void*));

  constexpr std::size_t kMix = compiler::SelectSize()
                                   .For64Bit(8)
                                   .For32Bit(4)
                                   .ForLibCpp64(8)
                                   .ForLibStdCpp32(4);
  EXPECT_EQ(kMix, sizeof(void*));
}

USERVER_NAMESPACE_END
