#pragma once

#include <string>
#include <unordered_set>

USERVER_NAMESPACE_BEGIN

namespace logging {

bool DynamicDebugShouldLog(std::string_view location);

bool DynamicDebugShouldLogRelative(std::string_view location);

void SetDynamicDebugLog(const std::string& location_relative, bool enable);

void RestrictDynamicDebugLocations(
    const std::unordered_set<std::string>& restrict_list);

void InitDynamicDebugLog();

std::unordered_set<std::string> GetDynamicDebugLocations();

}  // namespace logging

USERVER_NAMESPACE_END
