#pragma once

/// @file userver/utils/distances.hpp
/// @brief Distances between strings and nearest string suggestions

#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace impl {

std::string SuggestNameErrorMsg(std::optional<std::string_view> suggest_name);

}  // namespace impl

/// Returns the Levenshtein distance between two strings.
std::size_t GetLevenshteinDistance(std::string_view view1,
                                   std::string_view view2);

/// Returns the Damerau-Levenshtein distance between two strings.
std::size_t GetDamerauLevenshteinDistance(std::string_view view1,
                                          std::string_view view2);

/// Returns a nearest string for a key from a bunch of strings.
template <typename StringViews, typename DistanceFunc = std::size_t (*)(
                                    std::string_view, std::string_view)>
std::optional<std::string_view> GetNearestString(
    const StringViews& strings, std::string_view key, std::size_t max_distance,
    DistanceFunc distance_func = &utils::GetLevenshteinDistance) {
  static_assert(
      std::is_convertible_v<decltype(*std::begin(strings)), std::string_view>,
      "Parameter `strings` should be an iterable over strings.");

  std::optional<std::string_view> nearest_str;
  std::size_t min_distance = 0;
  for (const auto& str : strings) {
    std::string_view obj_view(str);
    std::size_t cur_distance = distance_func(key, obj_view);
    if (!nearest_str.has_value() || cur_distance < min_distance) {
      nearest_str = obj_view;
      min_distance = cur_distance;
    }
  }
  if (nearest_str.has_value() && min_distance <= max_distance) {
    return nearest_str;
  }
  return {};
}

/// Returns a suggestion for a key
template <typename StringViews>
std::string SuggestNearestName(const StringViews& strings,
                               std::string_view key) {
  static_assert(
      std::is_convertible_v<decltype(*std::begin(strings)), std::string_view>,
      "Parameter `strings` should be an iterable over strings.");
  constexpr std::size_t kMaxDistance = 3;
  return impl::SuggestNameErrorMsg(
      utils::GetNearestString(strings, key, kMaxDistance));
}

}  // namespace utils

USERVER_NAMESPACE_END
