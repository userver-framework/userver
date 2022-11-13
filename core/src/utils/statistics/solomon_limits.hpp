#pragma once

#include <array>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics::impl::solomon {

// These labels are always applied and cannot be set by user
inline constexpr std::array<std::string_view, 6> kReservedLabelNames = {
    "project", "cluster", "service", "host", "group", "sensor"};

// https://solomon.yandex-team.ru/docs/concepts/data-model#limits
// "application" is in commonLabels and can be overridden (nginx system metrics)
inline constexpr std::size_t kMaxLabels = 16 - kReservedLabelNames.size() - 1;
inline constexpr std::size_t kMaxLabelNameLen = 31;
inline constexpr std::size_t kMaxLabelValueLen = 200;

}  // namespace utils::statistics::impl::solomon

USERVER_NAMESPACE_END
