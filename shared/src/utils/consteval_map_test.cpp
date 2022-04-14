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
      "hello",
      "world",
      "a",
      "b",
      "c",
      "d",
      "e",
      "f",
      "z",
  });

  static_assert(mapping.Contains("hello"));
  static_assert(mapping.Contains("z"));
  static_assert(mapping.Contains("a"));
  static_assert(!mapping.Contains("bla"));
}

TEST(ConstevalMap, SetLinearSearch) {
  constexpr auto mapping = utils::MakeConsinitSet<std::string_view>({
      "hello",
      "a",
      "z",
  });

  static_assert(mapping.Contains("hello"));
  static_assert(mapping.Contains("z"));
  static_assert(mapping.Contains("a"));
  static_assert(!mapping.Contains("bla"));
}

USERVER_NAMESPACE_END
