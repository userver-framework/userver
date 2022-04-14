#include <userver/utils/consteval_map.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

TEST(ConstevalMap, MapBinarySearch) {
  constexpr auto mapping = utils::MakeConsinitMap<std::string_view, int>({
      {"hello", 1},
      {"world", 2},
      {"a", 3},
      {"b", 4},
      {"c", 5},
      {"d", 6},
      {"e", 7},
      {"f", 8},
      {"z", 42},
      {"aaaaaaaaaaaaaaaa_hello", 1},
      {"aaaaaaaaaaaaaaaa_world", 2},
      {"aaaaaaaaaaaaaaaa_a", 3},
      {"aaaaaaaaaaaaaaaa_b", 4},
      {"aaaaaaaaaaaaaaaa_c", 5},
      {"aaaaaaaaaaaaaaaa_d", 6},
      {"aaaaaaaaaaaaaaaa_e", 7},
      {"aaaaaaaaaaaaaaaa_f", 8},
      {"aaaaaaaaaaaaaaaa_f1", 8},
      {"aaaaaaaaaaaaaaaa_f2", 8},
      {"aaaaaaaaaaaaaaaa_f3", 8},
      {"aaaaaaaaaaaaaaaa_f4", 8},
      {"aaaaaaaaaaaaaaaa_f5", 8},
      {"aaaaaaaaaaaaaaaa_f6", 8},
      {"aaaaaaaaaaaaaaaa_f7", 8},
      {"aaaaaaaaaaaaaaaa_f8", 8},
      {"aaaaaaaaaaaaaaaa_f9", 8},
      {"aaaaaaaaaaaaaaaa_z", 42},
      {"aaaaaaaaaaaaaaaa_z1", 42},
      {"aaaaaaaaaaaaaaaa_z2", 42},
      {"aaaaaaaaaaaaaaaa_z3", 42},
      {"aaaaaaaaaaaaaaaa_z4", 42},
      {"aaaaaaaaaaaaaaaa_z5", 42},
      {"aaaaaaaaaaaaaaaa_z6", 42},
      {"aaaaaaaaaaaaaaaa_z7", 42},
      {"aaaaaaaaaaaaaaaa_z8", 42},
      {"aaaaaaaaaaaaaaaa_z9", 42},
      {"aaaaaaaaaaaaaaaa_x", 42},
      {"aaaaaaaaaaaaaaaa_x1", 42},
      {"aaaaaaaaaaaaaaaa_x2", 42},
      {"aaaaaaaaaaaaaaaa_x3", 42},
      {"aaaaaaaaaaaaaaaa_x4", 42},
      {"aaaaaaaaaaaaaaaa_x5", 42},
      {"aaaaaaaaaaaaaaaa_x6", 42},
      {"aaaaaaaaaaaaaaaa_x7", 42},
      {"aaaaaaaaaaaaaaaa_x8", 42},
      {"aaaaaaaaaaaaaaaa_x9", 42},
      {"bbbbbbbbbbbbbbb_hello", 1},
      {"bbbbbbbbbbbbbbb_world", 2},
      {"bbbbbbbbbbbbbbb_a", 3},
      {"bbbbbbbbbbbbbbb_b", 4},
      {"bbbbbbbbbbbbbbb_c", 5},
      {"bbbbbbbbbbbbbbb_d", 6},
      {"bbbbbbbbbbbbbbb_e", 7},
      {"bbbbbbbbbbbbbbb_f", 8},
      {"bbbbbbbbbbbbbbb_f1", 8},
      {"bbbbbbbbbbbbbbb_f2", 8},
      {"bbbbbbbbbbbbbbb_f3", 8},
      {"bbbbbbbbbbbbbbb_f4", 8},
      {"bbbbbbbbbbbbbbb_f5", 8},
      {"bbbbbbbbbbbbbbb_f6", 8},
      {"bbbbbbbbbbbbbbb_f7", 8},
      {"bbbbbbbbbbbbbbb_f8", 8},
      {"bbbbbbbbbbbbbbb_f9", 8},
      {"bbbbbbbbbbbbbbb_z", 42},
      {"bbbbbbbbbbbbbbb_z1", 42},
      {"bbbbbbbbbbbbbbb_z2", 42},
      {"bbbbbbbbbbbbbbb_z3", 42},
      {"bbbbbbbbbbbbbbb_z4", 42},
      {"bbbbbbbbbbbbbbb_z5", 42},
      {"bbbbbbbbbbbbbbb_z6", 42},
      {"bbbbbbbbbbbbbbb_z7", 42},
      {"bbbbbbbbbbbbbbb_z8", 42},
      {"bbbbbbbbbbbbbbb_z9", 42},
      {"bbbbbbbbbbbbbbb_x", 42},
      {"bbbbbbbbbbbbbbb_x1", 42},
      {"bbbbbbbbbbbbbbb_x2", 42},
      {"bbbbbbbbbbbbbbb_x3", 42},
      {"bbbbbbbbbbbbbbb_x4", 42},
      {"bbbbbbbbbbbbbbb_x5", 42},
      {"bbbbbbbbbbbbbbb_x6", 42},
      {"bbbbbbbbbbbbbbb_x7", 42},
      {"bbbbbbbbbbbbbbb_x8", 42},
      {"bbbbbbbbbbbbbbb_x9", 42},
  });

  static_assert(mapping.Contains("hello"));
  static_assert(mapping.Contains("z"));
  static_assert(mapping.Contains("a"));
  static_assert(!mapping.Contains("bla"));

  static_assert(*mapping.FindOrNullptr("hello") == 1);
  static_assert(*mapping.FindOrNullptr("z") == 42);
  static_assert(*mapping.FindOrNullptr("a") == 3);
  static_assert(!mapping.FindOrNullptr("bla"));
}

TEST(ConstevalMap, MapLinearSearch) {
  constexpr auto mapping = utils::MakeConsinitMap<std::string_view, int>({
      {"hello", 1},
      {"a", 3},
      {"z", 42},
  });

  static_assert(mapping.Contains("hello"));
  static_assert(mapping.Contains("z"));
  static_assert(mapping.Contains("a"));
  static_assert(!mapping.Contains("bla"));

  static_assert(*mapping.FindOrNullptr("hello") == 1);
  static_assert(*mapping.FindOrNullptr("z") == 42);
  static_assert(*mapping.FindOrNullptr("a") == 3);
  static_assert(!mapping.FindOrNullptr("bla"));
}

TEST(ConstevalMap, SetBinarySearch) {
  constexpr auto mapping = utils::MakeConsinitSet<std::string_view>({
      {"hello"},
      {"world"},
      {"a"},
      {"b"},
      {"c"},
      {"d"},
      {"e"},
      {"f"},
      {"z"},
      {"aaaaaaaaaaaaaaaa_hello"},
      {"aaaaaaaaaaaaaaaa_world"},
      {"aaaaaaaaaaaaaaaa_a"},
      {"aaaaaaaaaaaaaaaa_b"},
      {"aaaaaaaaaaaaaaaa_c"},
      {"aaaaaaaaaaaaaaaa_d"},
      {"aaaaaaaaaaaaaaaa_e"},
      {"aaaaaaaaaaaaaaaa_f"},
      {"aaaaaaaaaaaaaaaa_f1"},
      {"aaaaaaaaaaaaaaaa_f2"},
      {"aaaaaaaaaaaaaaaa_f3"},
      {"aaaaaaaaaaaaaaaa_f4"},
      {"aaaaaaaaaaaaaaaa_f5"},
      {"aaaaaaaaaaaaaaaa_f6"},
      {"aaaaaaaaaaaaaaaa_f7"},
      {"aaaaaaaaaaaaaaaa_f8"},
      {"aaaaaaaaaaaaaaaa_f9"},
      {"aaaaaaaaaaaaaaaa_z"},
      {"aaaaaaaaaaaaaaaa_z1"},
      {"aaaaaaaaaaaaaaaa_z2"},
      {"aaaaaaaaaaaaaaaa_z3"},
      {"aaaaaaaaaaaaaaaa_z4"},
      {"aaaaaaaaaaaaaaaa_z5"},
      {"aaaaaaaaaaaaaaaa_z6"},
      {"aaaaaaaaaaaaaaaa_z7"},
      {"aaaaaaaaaaaaaaaa_z8"},
      {"aaaaaaaaaaaaaaaa_z9"},
      {"aaaaaaaaaaaaaaaa_x"},
      {"aaaaaaaaaaaaaaaa_x1"},
      {"aaaaaaaaaaaaaaaa_x2"},
      {"aaaaaaaaaaaaaaaa_x3"},
      {"aaaaaaaaaaaaaaaa_x4"},
      {"aaaaaaaaaaaaaaaa_x5"},
      {"aaaaaaaaaaaaaaaa_x6"},
      {"aaaaaaaaaaaaaaaa_x7"},
      {"aaaaaaaaaaaaaaaa_x8"},
      {"aaaaaaaaaaaaaaaa_x9"},
      {"bbbbbbbbbbbbbbb_hello"},
      {"bbbbbbbbbbbbbbb_world"},
      {"bbbbbbbbbbbbbbb_a"},
      {"bbbbbbbbbbbbbbb_b"},
      {"bbbbbbbbbbbbbbb_c"},
      {"bbbbbbbbbbbbbbb_d"},
      {"bbbbbbbbbbbbbbb_e"},
      {"bbbbbbbbbbbbbbb_f"},
      {"bbbbbbbbbbbbbbb_f1"},
      {"bbbbbbbbbbbbbbb_f2"},
      {"bbbbbbbbbbbbbbb_f3"},
      {"bbbbbbbbbbbbbbb_f4"},
      {"bbbbbbbbbbbbbbb_f5"},
      {"bbbbbbbbbbbbbbb_f6"},
      {"bbbbbbbbbbbbbbb_f7"},
      {"bbbbbbbbbbbbbbb_f8"},
      {"bbbbbbbbbbbbbbb_f9"},
      {"bbbbbbbbbbbbbbb_z"},
      {"bbbbbbbbbbbbbbb_z1"},
      {"bbbbbbbbbbbbbbb_z2"},
      {"bbbbbbbbbbbbbbb_z3"},
      {"bbbbbbbbbbbbbbb_z4"},
      {"bbbbbbbbbbbbbbb_z5"},
      {"bbbbbbbbbbbbbbb_z6"},
      {"bbbbbbbbbbbbbbb_z7"},
      {"bbbbbbbbbbbbbbb_z8"},
      {"bbbbbbbbbbbbbbb_z9"},
      {"bbbbbbbbbbbbbbb_x"},
      {"bbbbbbbbbbbbbbb_x1"},
      {"bbbbbbbbbbbbbbb_x2"},
      {"bbbbbbbbbbbbbbb_x3"},
      {"bbbbbbbbbbbbbbb_x4"},
      {"bbbbbbbbbbbbbbb_x5"},
      {"bbbbbbbbbbbbbbb_x6"},
      {"bbbbbbbbbbbbbbb_x7"},
      {"bbbbbbbbbbbbbbb_x8"},
      {"bbbbbbbbbbbbbbb_x9"},
  });

  static_assert(mapping.Contains("hello"));
  static_assert(mapping.Contains("z"));
  static_assert(mapping.Contains("a"));
  static_assert(mapping.Contains("bbbbbbbbbbbbbbb_x9"));
  static_assert(mapping.Contains("bbbbbbbbbbbbbbb_f2"));
  static_assert(!mapping.Contains("bla"));

  for (auto value : mapping) {
    EXPECT_TRUE(mapping.Contains(value));
  }
}

TEST(ConstevalMap, SetLinearSearch) {
  constexpr auto mapping = utils::MakeConsinitSet<std::string_view>({
      {"hello"},
      {"a"},
      {"z"},
  });

  static_assert(mapping.Contains("hello"));
  static_assert(mapping.Contains("z"));
  static_assert(mapping.Contains("a"));
  static_assert(!mapping.Contains("bla"));
}

USERVER_NAMESPACE_END
