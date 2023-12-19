#include <gtest/gtest.h>

#include <optional>

#include <userver/http/header_map.hpp>
#include <userver/utils/enumerate.hpp>

#include <http/header_map/map.hpp>

#include <userver/internal/http/header_map_tests_helper.hpp>

USERVER_NAMESPACE_BEGIN

namespace http::headers {

namespace {

// if h1 % 2^k == h2 % 2^k, then h1 % 2^(k - 1) == h2 % 2^(k - 1) - we just
// discard the highest bit, all other bits stay the same.
// Mod 2048 with 800 collisions is more than enough to trigger map anti-hashdos.
constexpr std::size_t kMod = 1 << 11;
constexpr std::string_view kAlphabet{"abcdefghijklmnopqrstuvwxyz0123456789"};
constexpr std::size_t kCollisionsCount = 800;
constexpr std::size_t kStringLen = [] {
  std::size_t p = 1;
  for (std::size_t len = 1;; ++len) {
    p *= kAlphabet.size();
    if (p >= kCollisionsCount * kMod) {
      return len;
    }
  }
}();

template <typename Action>
void DoGenerateStrings(std::string& current, const Action& action,
                       std::size_t depth = 0) {
  if (depth == kStringLen) {
    action(current);
    return;
  }

  for (const auto c : kAlphabet) {
    current[depth] = c;
    DoGenerateStrings(current, action, depth + 1);
  }
}

std::vector<std::string> GenerateCollisions(const HeaderMap& map) {
  // There are two ways to generate collisions:
  // 1. Brute force all strings of length, say, 4. This relies on us knowing
  // that map resizes in powers of 2.
  // 2. Generate MurMur universal multicollisions. This relies on us knowing
  // that MurMur is used.
  //
  // 2. is way more complicated than 1. and still relies on some assumptions, so
  // why bother.

  const auto& danger = TestsHelper::GetMapDanger(map);

  std::vector<std::size_t> cnt(kMod, 0);

  std::string current;
  current.resize(kStringLen);

  DoGenerateStrings(current, [&danger, &cnt](const std::string& s) {
    const auto hash = danger.HashKey(s);
    ++cnt[hash % kMod];
  });

  std::vector<std::string> result{};
  result.reserve(kCollisionsCount);

  for (const auto [index, count] : utils::enumerate(cnt)) {
    if (count >= kCollisionsCount) {
      DoGenerateStrings(
          current, [&danger, &result, remainder = index](const std::string& s) {
            const auto hash = danger.HashKey(s);
            if (hash % kMod == remainder && result.size() < kCollisionsCount) {
              result.push_back(s);
            }
          });
      return result;
    }
  }

  ADD_FAILURE() << "Failed to generate collisions";

  return {};
}

void ValidateCollisions(const HeaderMap& map,
                        const std::vector<std::string>& collisions) {
  const auto& danger = TestsHelper::GetMapDanger(map);

  std::optional<std::size_t> common_hash{};
  for (const auto& s : collisions) {
    const auto hash = danger.HashKey(s) % kMod;

    ASSERT_EQ(common_hash.value_or(hash), hash);

    common_hash.emplace(hash);
  }
}

}  // namespace

TEST(HeaderMapHashDos, ModuloCollisions) {
  HeaderMap map{};

  const auto collisions = GenerateCollisions(map);
  ASSERT_GE(collisions.size(), kCollisionsCount);

  ValidateCollisions(map, collisions);

  for (const auto& s : collisions) {
    map.emplace(s, "1");
  }

  EXPECT_TRUE(TestsHelper::GetMapDanger(map).IsRed());
  EXPECT_LT(TestsHelper::GetMapMaxDisplacement(map), 10);
}

TEST(HeaderMapHashDos, HashAbuse) {
  constexpr std::size_t kCollisionsLen = 10;
  static_assert((1 << kCollisionsLen) >= kCollisionsCount);

  HeaderMap map{};
  const auto& danger = TestsHelper::GetMapDanger(map);

  EXPECT_EQ(danger.HashKey("a"), danger.HashKey("A"));
  EXPECT_EQ(danger.HashKey("["), danger.HashKey("{"));

  std::vector<std::string> collisions;
  collisions.reserve(kCollisionsCount);
  for (std::size_t i = 0; i < kCollisionsCount; ++i) {
    std::string collision(kCollisionsLen, '[');
    for (std::size_t j = 0; j < kCollisionsLen; ++j) {
      if ((i >> j) & 1) {
        collision[j] = '{';
      }
    }
    collisions.push_back(std::move(collision));
  }

  ValidateCollisions(map, collisions);

  for (const auto& s : collisions) {
    map.emplace(s, "1");
  }
  EXPECT_TRUE(TestsHelper::GetMapDanger(map).IsRed());
  EXPECT_LT(TestsHelper::GetMapMaxDisplacement(map), 10);
  EXPECT_EQ(map.size(), kCollisionsCount);
  // hashing changed to siphash
  EXPECT_NE(danger.HashKey("["), danger.HashKey("{"));
}

}  // namespace http::headers

USERVER_NAMESPACE_END
