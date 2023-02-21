#pragma once

#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

USERVER_NAMESPACE_BEGIN

namespace utils {

enum class DistanceType { kLevenshtein, kDamerauLevenshtein };

std::size_t GetLevenshteinDistance(std::string_view view1,
                                   std::string_view view2);

std::size_t GetDamerauLevenshteinDistance(std::string_view view1,
                                          std::string_view view2);

template <typename StringViews,
          std::enable_if_t<
              std::is_convertible_v<typename StringViews::iterator::value_type,
                                    std::string_view>,
              bool> = true>
std::optional<std::string_view> GetNearestString(
    const StringViews& strings, std::string_view key, std::size_t max_distance,
    DistanceType distance_type = DistanceType::kLevenshtein) {
  using DistanceFunc = std::size_t (*)(std::string_view, std::string_view);
  DistanceFunc distance_func = nullptr;
  switch (distance_type) {
    case DistanceType::kLevenshtein:
      distance_func = GetLevenshteinDistance;
      break;
    case DistanceType::kDamerauLevenshtein:
      distance_func = GetDamerauLevenshteinDistance;
      break;
    default:
      throw std::runtime_error("Current type of distance is not supported.");
  }

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

std::string SuggestNameErrorMsg(std::optional<std::string_view> suggest_name);

template <typename StringViews,
          std::enable_if_t<
              std::is_convertible_v<typename StringViews::iterator::value_type,
                                    std::string_view>,
              bool> = true>
std::string SuggestNearestName(const StringViews& strings,
                               std::string_view key) {
  constexpr std::size_t kMaxDistance = 3;
  return SuggestNameErrorMsg(GetNearestString(strings, key, kMaxDistance));
}

}  // namespace utils

USERVER_NAMESPACE_END
