#include <userver/utils/box.hpp>

#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>

#include <gtest/gtest.h>

#include <userver/utest/assert_macros.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

struct RecursionStarterPack;
struct PackSection;

bool operator==(const RecursionStarterPack&, const RecursionStarterPack&);
bool operator==(const PackSection&, const PackSection&);
bool operator!=(const PackSection&, const PackSection&);

/// [sample]
struct RecursionStarterPack {
  std::string structs;
  std::string strings;
  utils::Box<PackSection> whats_in_this_section;
};

struct PackSection {
  std::string box;
  std::optional<RecursionStarterPack> here_we_go_again;
};

TEST(UtilsBox, Sample) {
  PackSection pack{
      "box",
      {{
          "structs",
          "strings",
          utils::Box<PackSection>{{
              "box",
              std::nullopt,
          }},
      }},
  };

  // Classic value semantics test.
  auto another_pack = pack;
  EXPECT_EQ(another_pack, pack);
  another_pack.here_we_go_again->structs = "modified structs";
  EXPECT_NE(another_pack, pack);
  EXPECT_EQ(pack.here_we_go_again->structs, "structs");
}
/// [sample]

bool operator==(const RecursionStarterPack& lhs,
                const RecursionStarterPack& rhs) {
  return std::tie(lhs.structs, lhs.strings, lhs.whats_in_this_section) ==
         std::tie(rhs.structs, rhs.strings, rhs.whats_in_this_section);
}

bool operator==(const PackSection& lhs, const PackSection& rhs) {
  return std::tie(lhs.box, lhs.here_we_go_again) ==
         std::tie(rhs.box, rhs.here_we_go_again);
}

bool operator!=(const PackSection& lhs, const PackSection& rhs) {
  return !(lhs == rhs);
}

struct NonDefaulted final {
  explicit NonDefaulted(int) {}
};

}  // namespace

TEST(UtilsBox, ConstructionSFINAE) {
  static_assert(std::is_default_constructible_v<utils::Box<int>>);

  static_assert(std::is_constructible_v<utils::Box<std::string>, const char*>);
  static_assert(
      std::is_constructible_v<utils::Box<std::string>, std::string_view>);
  static_assert(
      std::is_constructible_v<utils::Box<std::string>, std::size_t, char>);

  static_assert(std::is_constructible_v<utils::Box<NonDefaulted>, int>);
  static_assert(
      !std::is_constructible_v<utils::Box<NonDefaulted>, std::string>);
}

TEST(UtilsBox, Explicit) {
  static_assert(std::is_convertible_v<const char*, utils::Box<std::string>>);
  static_assert(
      !std::is_convertible_v<std::string_view, utils::Box<std::string>>);
  static_assert(!std::is_convertible_v<int, utils::Box<NonDefaulted>>);
}

TEST(UtilsBox, Assignment) {
  {
    utils::Box<std::string> was_normal = "foo";
    was_normal = "bar";
    EXPECT_EQ(*was_normal, "bar");
  }
  {
    utils::Box<std::string> was_moved_from = "foo";
    [[maybe_unused]] auto _ = std::move(was_moved_from);
    was_moved_from = "bar";
    EXPECT_EQ(*was_moved_from, "bar");
  }
}

namespace {

struct ThrowsOnAssignment final {
  explicit ThrowsOnAssignment(int value) : value(value) {}

  ThrowsOnAssignment(const ThrowsOnAssignment&) = default;

  // NOLINTNEXTLINE(cert-oop54-cpp)
  ThrowsOnAssignment& operator=(const ThrowsOnAssignment&) {
    throw std::runtime_error("Test");
  }

  int value;
};

}  // namespace

TEST(UtilsBox, ExceptionDuringAssignment) {
  utils::Box<ThrowsOnAssignment> box{10};
  UEXPECT_THROW_MSG(box = ThrowsOnAssignment{20}, std::runtime_error, "Test");
  EXPECT_EQ(box->value, 10);
}

USERVER_NAMESPACE_END
