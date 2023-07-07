#include <benchmark/benchmark.h>

#include <string>
#include <vector>

#include <userver/http/header_map.hpp>

#include <userver/internal/http/header_map_tests_helper.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr std::string_view kAlphabet = "abcdefghijklmnopqrstuvwxyz";
constexpr std::size_t kHashMask = (1UL << 15) - 1;
constexpr std::size_t kMaxBlockSize = 120;
constexpr std::size_t kMaxBlocksCount = 25;

using CollisionsBlock = std::vector<std::string>;

std::vector<CollisionsBlock> GenerateCollisions() {
  std::vector<CollisionsBlock> result(kMaxBlocksCount);
  for (auto& block : result) {
    block.reserve(kMaxBlockSize);
  }

  for (const auto a : kAlphabet) {
    for (const auto b : kAlphabet) {
      for (const auto c : kAlphabet) {
        for (const auto d : kAlphabet) {
          for (const auto e : kAlphabet) {
            std::string s{a, b, c, d, e};
            const auto hash =
                http::headers::impl::UnsafeConstexprHasher{}(s)&kHashMask;
            if (hash % kMaxBlockSize == 0 &&
                hash / kMaxBlockSize < kMaxBlocksCount) {
              auto& block = result[hash / kMaxBlockSize];
              if (block.size() < kMaxBlockSize) {
                block.push_back(std::move(s));
              }
            }
          }
        }
      }
    }
  }

  return result;
}

const auto kCollisionBlocks = GenerateCollisions();

}  // namespace

// We have 120 * 25 = 3000 headers,
// split into 25 chunks of 120 colliding headers (which is the worst-case
// scenario complexity-wise, a bit more than 120 will trigger HeaderMap to
// switch into red state and make collisions generation impossible).
// Every header name is 5 bytes, so a header line in http request looks like
// this: 'abcde: 1\r\n' and takes 10 bytes, 10 bytes times 3000 headers ~= 30Kb,
// which approaches default (yet very generous) headers size limits for proxies.
//
// On my machine this benchmark takes ~1ms to insert these headers, so an
// attacker will need 30Mb/s of bandwidth to exhaust just one core...
// If someone has enough firepower to do that HeaderMap collisions resolution
// schema is the last problem we have to address.
void HeaderMapWorstCaseCollisionsBenchmark(benchmark::State& state) {
  std::size_t index = 0;
  for (const auto& block : kCollisionBlocks) {
    if (block.size() != kMaxBlockSize) {
      state.SkipWithError("Benchmark doesn't generate collisions properly");
      return;
    }

    std::optional<std::size_t> common_block_hash{};
    for (const auto& s : block) {
      const auto h = http::headers::impl::UnsafeConstexprHasher{}(s)&kHashMask;
      if (common_block_hash.value_or(h) != index * kMaxBlockSize) {
        state.SkipWithError("Benchmark doesn't generate collisions properly");
        return;
      }
      common_block_hash.emplace(h);
    }
    ++index;
  }

  for (auto _ : state) {
    http::headers::HeaderMap map{};

    for (const auto& block : kCollisionBlocks) {
      for (const auto& collision : block) {
        map.InsertOrAppend(collision, "1");
      }
      if (http::headers::TestsHelper::GetMapDanger(map).IsRed()) {
        state.SkipWithError("Map switched into red state unexpectedly");
      }
    }

    if (map.size() != kCollisionBlocks.size() * kMaxBlockSize) {
      state.SkipWithError("Map implementation is broken");
    }

    if (http::headers::TestsHelper::GetMapMaxDisplacement(map) + 1 !=
        kMaxBlockSize) {
      state.SkipWithError("Benchmark assumptions about collisions are not met");
    }
  }
}
BENCHMARK(HeaderMapWorstCaseCollisionsBenchmark);

void HeaderMapEraseBenchmark(benchmark::State& state) {
  const auto headers = [] {
    constexpr std::size_t kHeadersCount = 1000;

    std::vector<std::string> headers;
    headers.reserve(kHeadersCount);

    std::string current_header{kAlphabet};

    for (std::size_t i = 0; i < kHeadersCount; ++i) {
      headers.push_back(current_header);
      std::next_permutation(current_header.begin(), current_header.end());
    }

    return headers;
  }();

  const auto initial_map = [&headers] {
    http::headers::HeaderMap map{};
    map.reserve(headers.size());

    for (const auto& h : headers) {
      map.emplace(h, "1");
    }

    return map;
  }();

  if (initial_map.size() != headers.size()) {
    state.SkipWithError("Map implementation is broken");
    return;
  }
  if (http::headers::TestsHelper::GetMapDanger(initial_map).IsRed()) {
    state.SkipWithError("Map unexpectedly changed into red state");
    return;
  }

  std::vector<http::headers::PredefinedHeader> predefined_headers{
      headers.begin(), headers.end()};
  for (auto _ : state) {
    auto map = initial_map;
    for (const auto& h : predefined_headers) {
      map.erase(h);
    }
    if (!map.empty()) {
      state.SkipWithError("Map implementation is broken");
    }
  }
}
BENCHMARK(HeaderMapEraseBenchmark);

USERVER_NAMESPACE_END
