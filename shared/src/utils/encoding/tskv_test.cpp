#include <userver/utils/encoding/tskv.hpp>

#include <algorithm>
#include <optional>

#include <gtest/gtest.h>

#include <userver/utils/enumerate.hpp>
#include <utils/encoding/tskv_testdata_bin.hpp>

using namespace std::string_literals;
using namespace std::string_view_literals;

USERVER_NAMESPACE_BEGIN

TEST(Tskv, NoEscapeQuote) {
  const auto* str = R"({ "tasks" : [  ] })";
  for (auto mode : {utils::encoding::EncodeTskvMode::kValue,
                    utils::encoding::EncodeTskvMode::kKey}) {
    std::string result;
    utils::encoding::EncodeTskv(result, str, mode);
    EXPECT_EQ(str, result);
  }
}

TEST(Tskv, TAXICOMMON1362) {
  const std::string_view str(reinterpret_cast<const char*>(tskv_test::data_bin),
                             sizeof(tskv_test::data_bin));
  std::string result;
  utils::encoding::EncodeTskv(result, str,
                              utils::encoding::EncodeTskvMode::kValue);
  EXPECT_TRUE(result.find("PNG") != std::string::npos) << "Result: " << result;

  EXPECT_TRUE(result.find(tskv_test::ascii_part) != std::string::npos)
      << "Result: " << result;

  EXPECT_EQ(0, std::count(result.begin(), result.end(), '\n'))
      << "Result: " << result;
  EXPECT_EQ(0, std::count(result.begin(), result.end(), '\t'))
      << "Result: " << result;
  EXPECT_EQ(0, std::count(result.begin(), result.end(), '\0'))
      << "Result: " << result;
}

namespace {

struct EncodeTskvTestParam final {
  std::string source;
  std::string expected_encoded_result;
};

struct EncodeTskvTestSet final {
  std::string name;
  std::vector<EncodeTskvTestParam> tests;
};

auto MakeRepeatedTests(std::string_view source,
                       std::string_view expected_encoded_result,
                       std::size_t times) {
  std::vector<EncodeTskvTestParam> result;
  std::string total_source;
  std::string total_expected_encoded_result;
  for (std::size_t i = 0; i <= times; ++i) {
    total_source += source;
    total_expected_encoded_result += expected_encoded_result;
    result.push_back({total_source, total_expected_encoded_result});
  }
  return result;
}

constexpr std::size_t kMaxBlockAlignment = 32;

std::size_t BytesToNextBlock(const char* ptr) {
  return -reinterpret_cast<std::uintptr_t>(ptr) % kMaxBlockAlignment;
}

auto MakeTskvTests() {
  return std::vector<EncodeTskvTestSet>{
      {
          "unmodified",
          {
              {"", ""},
              {"42", "42"},
              {"(null)", "(null)"},
              {R"({"tasks": []})", R"({"tasks": []})"},
              {"http.port.ipv4", "http.port.ipv4"},
          },
      },
      {
          "modified",
          {
              {"\\", "\\\\"},
              {"line 1\nline 2", "line 1\\nline 2"},
              {"FF161300\t0x0", "FF161300\\t0x0"},
              {"\r\n\0\t\\"s, R"(\r\n\0\t\\)"s},
          },
      },
      {"repeated-normal-char", MakeRepeatedTests("a"sv, "a"sv, 64)},
      {"repeated-special-char", MakeRepeatedTests("\0"sv, "\\0", 64)},
      {"repeated-backslash", MakeRepeatedTests("\\", "\\\\", 64)},
      {"repeated-special-char-sequence",
       MakeRepeatedTests("\r\n\0\t\\"sv, R"(\r\n\0\t\\)"sv, 8)},
  };
}

template <typename Encoder>
void DoTestValueBuffered(const std::string& test_set_name,
                         std::size_t test_id_base1, std::size_t offset,
                         char filler_char, const std::string& source,
                         const std::string& expected_encoded_result) {
  // By moving string_view to different offsets within the same 'source'
  // buffer, we test the behavior on chunk borders.
  std::string extended_source;
  extended_source.reserve(source.size() + 3 * kMaxBlockAlignment);
  const auto offset_before_block = BytesToNextBlock(extended_source.data());
  extended_source += std::string(offset_before_block, filler_char);
  extended_source += std::string(offset, filler_char);
  extended_source += source;
  extended_source += std::string(kMaxBlockAlignment, filler_char);
  const std::string_view embedded_source(
      extended_source.data() + offset_before_block + offset, source.size());

  std::string result;
  utils::encoding::impl::tskv::EncodeFullyBuffered<Encoder>(
      result, embedded_source, utils::encoding::EncodeTskvMode::kValue);

  EXPECT_EQ(result, expected_encoded_result)
      << "test_set_name=\"" << test_set_name
      << "\" test_id_base1=" << test_id_base1 << " offset=" << offset
      << " filler_char='" << filler_char << "'"
      << " source=\"" << source << "\"";
}

template <typename T>
class EncodeTskv : public ::testing::Test {
 public:
  using Encoder = T;
};

using EncoderList = ::testing::Types<
#ifdef __AVX2__
    utils::encoding::impl::tskv::EncoderAvx2,
#endif
#ifdef __SSSE3__
    utils::encoding::impl::tskv::EncoderSsse3,
#endif
#ifdef __SSE2__
    utils::encoding::impl::tskv::EncoderSse2,
#endif
    utils::encoding::impl::tskv::EncoderStd>;

}  // namespace

TYPED_TEST_SUITE(EncodeTskv, EncoderList);

TYPED_TEST(EncodeTskv, ValueBuffered) {
  const auto test_sets = MakeTskvTests();

  for (const auto& test_set : test_sets) {
    for (const auto& [i, test] : utils::enumerate(test_set.tests)) {
      const std::size_t test_id_base1 = i + 1;
      for (std::size_t offset = 0; offset < kMaxBlockAlignment; ++offset) {
        for (const auto filler_char : {'\0', '\n', '\\'}) {
          DoTestValueBuffered<typename TestFixture::Encoder>(
              test_set.name, test_id_base1, offset, filler_char, test.source,
              test.expected_encoded_result);
        }
      }
    }
  }
}

USERVER_NAMESPACE_END
