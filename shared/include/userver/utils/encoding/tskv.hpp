#pragma once

/// @file userver/utils/encoding/tskv.hpp
/// @brief Encoders, decoders and helpers for TSKV representations

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string_view>
#include <type_traits>

#ifdef __AVX2__
#include <immintrin.h>
#elif defined(__SSSE3__)
#include <tmmintrin.h>
#elif defined(__SSE2__)
#include <emmintrin.h>
#endif

#include <userver/utils/assert.hpp>

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_IMPL_FORCE_INLINE __attribute__((always_inline)) inline

#ifdef __clang__
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_IMPL_DISABLE_ASAN \
  __attribute__((no_sanitize_address, no_sanitize_memory))
#else
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_IMPL_DISABLE_ASAN __attribute__((no_sanitize_address))
#endif

USERVER_NAMESPACE_BEGIN

namespace utils::encoding {

constexpr inline char kTskvKeyValueSeparator = '=';
constexpr inline char kTskvPairsSeparator = '\t';

// kKeyReplacePeriod is for logging. Elastic has a long history of problems with
// periods in TSKV keys. For more info see:
// www.elastic.co/guide/en/elasticsearch/reference/2.4/dots-in-names.html
enum class EncodeTskvMode { kKey, kValue, kKeyReplacePeriod };

/// @brief Encode according to the TSKV rules, but without escaping the
/// quotation mark (").
/// @returns The iterator to after the inserted chars.
template <typename OutIter>
OutIter EncodeTskv(OutIter destination, char ch, EncodeTskvMode mode);

/// @brief Encode according to the TSKV rules, but without escaping the
/// quotation mark (").
/// @note New contents are appended at the end of `container`. Some extra memory
/// is reserved as necessary.
/// @tparam Container must be continuous and support at least the following
/// operations: 1) `c.data()` 2) `c.size()` 3) `c.resize(new_size)`
template <typename Container>
void EncodeTskv(Container& container, std::string_view str,
                EncodeTskvMode mode);

// ==================== Implementation follows ====================

// always_inline to eliminate 'mode' checks
template <typename OutIter>
USERVER_IMPL_FORCE_INLINE OutIter EncodeTskv(OutIter destination, char ch,
                                             EncodeTskvMode mode) {
  const bool is_key_encoding = (mode == EncodeTskvMode::kKey ||
                                mode == EncodeTskvMode::kKeyReplacePeriod);
  const auto append = [&destination](char ch) { *(destination++) = ch; };

  switch (ch) {
    case '\t':
      append('\\');
      append('t');
      break;
    case '\r':
      append('\\');
      append('r');
      break;
    case '\n':
      append('\\');
      append('n');
      break;
    case '\0':
      append('\\');
      append('0');
      break;
    case '\\':
      append('\\');
      append(ch);
      break;
    case '.':
      if (mode == EncodeTskvMode::kKeyReplacePeriod) {
        append('_');
        break;
      }
      [[fallthrough]];
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
    case 'G':
    case 'H':
    case 'I':
    case 'J':
    case 'K':
    case 'L':
    case 'M':
    case 'N':
    case 'O':
    case 'P':
    case 'Q':
    case 'R':
    case 'S':
    case 'T':
    case 'U':
    case 'V':
    case 'W':
    case 'X':
    case 'Y':
    case 'Z':
      if (is_key_encoding) {
        append(ch | 0x20);  // ch - 'A' + 'a'
        break;
      }
      [[fallthrough]];
    case '=':
      if (is_key_encoding) {
        append('\\');
        append(ch);
        break;
      }
      [[fallthrough]];
    default:
      append(ch);
      break;
  }
  return destination;
}

namespace impl::tskv {

template <std::size_t Alignment, typename T>
USERVER_IMPL_FORCE_INLINE T* AlignDown(T* ptr) noexcept {
  static_assert(Alignment % sizeof(T) == 0);
  return reinterpret_cast<T*>(reinterpret_cast<std::uintptr_t>(ptr) /
                              Alignment * Alignment);
}

template <std::size_t Alignment>
USERVER_IMPL_FORCE_INLINE const char* AssumeAligned(
    const char* block) noexcept {
  UASSERT(reinterpret_cast<std::uintptr_t>(block) % Alignment == 0);
  return static_cast<const char*>(__builtin_assume_aligned(block, Alignment));
}

constexpr auto MakeShuffleIndicesForRightShift() noexcept {
  constexpr std::size_t kShuffleWidth = 16;
  std::array<std::uint8_t, kShuffleWidth * 2> result{};
  for (auto& item : result) item = 0xf0;
  for (std::size_t i = 0; i < kShuffleWidth; ++i) result[i] = i;
  return result;
}

struct EncoderStd final {
  using Block = std::uint64_t;
  static constexpr std::size_t kBlockSize = sizeof(Block);

  // Sanitizers are disabled within the function, because the SIMD loads
  // may intentionally wander to uninitialized memory. The loads never touch
  // memory outside "our" cache lines, though.
  USERVER_IMPL_DISABLE_ASAN inline static auto LoadBlock(
      const char* block) noexcept {
    block = AssumeAligned<kBlockSize>(block);
    return *reinterpret_cast<const Block*>(block);
  }

  USERVER_IMPL_FORCE_INLINE static void CopyBlock(Block block,
                                                  std::size_t offset,
                                                  char* destination) noexcept {
    const auto cut_block = block >> (offset * 8);
    std::memcpy(destination, &cut_block, sizeof(cut_block));
  }

  USERVER_IMPL_FORCE_INLINE static bool MayNeedValueEscaping(
      Block block, std::size_t offset, std::size_t count) noexcept {
    char buffer[kBlockSize]{};
    std::memcpy(&buffer, &block, sizeof(block));
    for (const char c : std::string_view(buffer + offset, count)) {
      if (c <= '\r' || c == '\\') return true;
    }
    return false;
  }
};

#ifdef __SSE2__
struct EncoderSse2 {
  using Block = __m128i;
  static constexpr std::size_t kBlockSize = sizeof(Block);

  // Sanitizers are disabled within the function, because the SIMD loads
  // may intentionally wander to uninitialized memory. The loads never touch
  // memory outside "our" cache lines, though.
  USERVER_IMPL_DISABLE_ASAN inline static Block LoadBlock(
      const char* block) noexcept {
    block = AssumeAligned<kBlockSize>(block);
    return _mm_load_si128(reinterpret_cast<const Block*>(block));
  }

  USERVER_IMPL_FORCE_INLINE static void CopyBlock(Block block_contents,
                                                  std::size_t offset,
                                                  char* destination) noexcept {
    alignas(kBlockSize * 2) char storage[kBlockSize * 2]{};
    _mm_store_si128(reinterpret_cast<Block*>(&storage), block_contents);
    const auto cut_block =
        _mm_loadu_si128(reinterpret_cast<__m128i_u*>(&storage[offset]));
    _mm_storeu_si128(reinterpret_cast<__m128i_u*>(destination), cut_block);
  }

  USERVER_IMPL_FORCE_INLINE static bool MayNeedValueEscaping(
      Block block, std::size_t offset, std::size_t count) noexcept {
    // 'char c' may need TSKV value escaping iff c <= '\r' || c == '\\'
    // 16 lower bits of the mask contain may-need-escaping flag per block's char
    const auto may_need_escaping_mask = _mm_movemask_epi8(
        _mm_or_si128(_mm_cmpgt_epi8(_mm_set1_epi8('\r' + 1), block),
                     _mm_cmpeq_epi8(block, _mm_set1_epi8('\\'))));
    return static_cast<std::uint32_t>(
               static_cast<std::uint32_t>(may_need_escaping_mask) >>
               offset << (32 - count)) != 0;
  }
};
#endif

#ifdef __SSSE3__
struct EncoderSsse3 final : public EncoderSse2 {
  USERVER_IMPL_FORCE_INLINE static void CopyBlock(Block block,
                                                  std::size_t offset,
                                                  char* destination) noexcept {
    static constexpr auto kShuffleIdx = MakeShuffleIndicesForRightShift();
    const auto pos = _mm_loadu_si128(
        reinterpret_cast<const __m128i_u*>(&kShuffleIdx[offset]));
    const auto cut_block = _mm_shuffle_epi8(block, pos);
    _mm_storeu_si128(reinterpret_cast<__m128i_u*>(destination), cut_block);
  }
};
#endif

#ifdef __AVX2__
struct EncoderAvx2 final {
  using Block = __m256i;
  static constexpr std::size_t kBlockSize = sizeof(Block);

  // Sanitizers are disabled within the function, because the SIMD loads
  // may intentionally wander to uninitialized memory. The loads never touch
  // memory outside "our" cache lines, though.
  USERVER_IMPL_DISABLE_ASAN inline static Block LoadBlock(
      const char* block) noexcept {
    block = AssumeAligned<kBlockSize>(block);
    return _mm256_load_si256(reinterpret_cast<const Block*>(block));
  }

  USERVER_IMPL_FORCE_INLINE static void CopyBlock(Block block,
                                                  std::size_t offset,
                                                  char* destination) noexcept {
    alignas(kBlockSize * 2) char storage[kBlockSize * 2]{};
    _mm256_store_si256(reinterpret_cast<Block*>(&storage), block);
    const auto cut_block =
        _mm256_loadu_si256(reinterpret_cast<__m256i_u*>(&storage[offset]));
    _mm256_storeu_si256(reinterpret_cast<__m256i_u*>(destination), cut_block);
  }

  USERVER_IMPL_FORCE_INLINE static bool MayNeedValueEscaping(
      Block block, std::size_t offset, std::size_t count) noexcept {
    // 'char c' may need TSKV value escaping iff c <= '\r' || c == '\\'
    // 32 lower bits of the mask contain may-need-escaping flag per block's char
    const auto may_need_escaping_mask = _mm256_movemask_epi8(
        _mm256_or_si256(_mm256_cmpgt_epi8(_mm256_set1_epi8('\r' + 1), block),
                        _mm256_cmpeq_epi8(block, _mm256_set1_epi8('\\'))));
    return static_cast<std::uint32_t>(
               static_cast<std::uint32_t>(may_need_escaping_mask) >>
               offset << (32 - count)) != 0;
  }
};
#endif

#if defined(__AVX2__)
using SystemEncoder = EncoderAvx2;
#elif defined(__SSSE3__)
using SystemEncoder = EncoderSsse3;
#elif defined(__SSE2__)
using SystemEncoder = EncoderSse2;
#else
using SystemEncoder = EncoderStd;
#endif

// It is assumed that starting with the current output position, there is enough
// free space to fit the converted data, plus `PaddingSize` extra bytes of free
// space, which the encoder is free to fill with garbage.
template <typename Encoder>
constexpr std::size_t PaddingSize() {
  return Encoder::kBlockSize;
}

template <typename Encoder>
struct BufferPtr final {
  char* current{nullptr};
};

template <typename Encoder>
USERVER_IMPL_FORCE_INLINE BufferPtr<Encoder> AppendBlock(
    BufferPtr<Encoder> destination, typename Encoder::Block block,
    std::size_t offset, std::size_t count) noexcept {
  char* const old_current = destination.current;
  destination.current += count;
  Encoder::CopyBlock(block, offset, old_current);
  return destination;
}

// noinline to avoid code duplication for a cold path
template <typename Encoder>
[[nodiscard]] __attribute__((noinline)) BufferPtr<Encoder> EncodeValueEach(
    BufferPtr<Encoder> destination, std::string_view str) {
  for (const char c : str) {
    destination.current =
        encoding::EncodeTskv(destination.current, c, EncodeTskvMode::kValue);
  }
  return destination;
}

template <typename Encoder>
[[nodiscard]] USERVER_IMPL_FORCE_INLINE BufferPtr<Encoder> EncodeValueBlock(
    BufferPtr<Encoder> destination, const char* block, std::size_t offset,
    std::size_t count) {
  UASSERT(offset < Encoder::kBlockSize);
  UASSERT(offset + count <= Encoder::kBlockSize);
  block = AssumeAligned<Encoder::kBlockSize>(block);
  const auto block_contents = Encoder::LoadBlock(block);

  if (__builtin_expect(
          Encoder::MayNeedValueEscaping(block_contents, offset, count),
          false)) {
    destination = tskv::EncodeValueEach(
        destination, std::string_view(block + offset, count));
  } else {
    // happy path: the whole block does not need escaping
    destination = tskv::AppendBlock(destination, block_contents, offset, count);
  }

  return destination;
}

// BufferPtr must be passed around by value to avoid aliasing issues.
template <typename Encoder>
[[nodiscard]] __attribute__((noinline)) BufferPtr<Encoder> EncodeValue(
    BufferPtr<Encoder> destination, std::string_view str) {
  if (str.empty()) return destination;

  const char* const first_block = AlignDown<Encoder::kBlockSize>(str.data());
  const auto first_block_offset =
      static_cast<std::size_t>(str.data() - first_block);
  const auto first_block_count =
      std::min(Encoder::kBlockSize - first_block_offset, str.size());

  destination = tskv::EncodeValueBlock(destination, first_block,
                                       first_block_offset, first_block_count);

  const char* const last_block =
      AlignDown<Encoder::kBlockSize>(str.data() + str.size());

  if (last_block != first_block) {
    for (const char* current_block = first_block + Encoder::kBlockSize;
         current_block < last_block; current_block += Encoder::kBlockSize) {
      destination = tskv::EncodeValueBlock(destination, current_block, 0,
                                           Encoder::kBlockSize);
    }

    const auto last_block_count =
        static_cast<std::size_t>(str.data() + str.size() - last_block);
    if (last_block_count != 0) {
      destination =
          tskv::EncodeValueBlock(destination, last_block, 0, last_block_count);
    }
  }

  return destination;
}

template <typename Encoder>
[[nodiscard]] BufferPtr<Encoder> DoEncode(BufferPtr<Encoder> destination,
                                          std::string_view str,
                                          EncodeTskvMode mode) {
  if (mode == EncodeTskvMode::kValue) {
    return tskv::EncodeValue(destination, str);
  } else {
    for (const char c : str) {
      destination.current = encoding::EncodeTskv(destination.current, c, mode);
    }
    return destination;
  }
}

inline std::size_t MaxEncodedSize(std::size_t source_size) noexcept {
  return source_size * 2;
}

template <typename Encoder, typename Container>
void EncodeFullyBuffered(Container& container, std::string_view str,
                         EncodeTskvMode mode) {
  const auto old_size = container.size();
  container.resize(old_size + MaxEncodedSize(str.size()) +
                   PaddingSize<Encoder>());
  BufferPtr<Encoder> buffer_ptr{container.data() + old_size};

  buffer_ptr = tskv::DoEncode(buffer_ptr, str, mode);

  container.resize(buffer_ptr.current - container.data());
}

}  // namespace impl::tskv

template <typename Container>
void EncodeTskv(Container& container, std::string_view str,
                EncodeTskvMode mode) {
  impl::tskv::EncodeFullyBuffered<impl::tskv::SystemEncoder>(container, str,
                                                             mode);
}

/// @cond
inline bool ShouldKeyBeEscaped(std::string_view key) noexcept {
  for (const char ch : key) {
    switch (ch) {
      case '\t':
      case '\r':
      case '\n':
      case '\0':
      case '\\':
      case '.':
      case '=':
        return true;
      default:
        if ('A' <= ch && ch <= 'Z') return true;
        break;
    }
  }
  return false;
}
/// @endcond

}  // namespace utils::encoding

USERVER_NAMESPACE_END

#undef USERVER_IMPL_FORCE_INLINE
#undef USERVER_IMPL_DONT_SANITIZE
