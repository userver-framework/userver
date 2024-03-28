#pragma once

#include <chrono>
#include <optional>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

std::chrono::system_clock::time_point TransformToTimePoint(
    std::chrono::seconds seconds);

std::optional<std::chrono::system_clock::time_point> TransformToTimePoint(
    std::optional<std::chrono::seconds> seconds);

std::chrono::seconds TransformToSeconds(
    std::chrono::system_clock::time_point time_point);

std::optional<std::chrono::seconds> TransformToSeconds(
    std::optional<std::chrono::system_clock::time_point> time_point);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
