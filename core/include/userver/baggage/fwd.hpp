#pragma once

/// @file userver/baggage/fwd.hpp
/// @brief Forward declarations of baggage and baggage helpers classes

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace baggage {

class BaggageManagerComponent;
class BaggageManager;
class Baggage;
class BaggageEntry;
class BaggageEntryProperty;

using BaggageProperties = std::vector<std::pair<std::string, std::optional<std::string>>>;

}  // namespace baggage
