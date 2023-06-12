#pragma once

/// @file userver/utils/statistics/fwd.hpp
/// @brief Forward declarations for statistics entities

#include <memory>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

// NOLINTNEXTLINE(bugprone-forward-declaration-namespace)
class Storage;

// TODO remove
struct StatisticsRequest;

// NOLINTNEXTLINE(bugprone-forward-declaration-namespace)
class Request;

class Entry;
class Writer;

class MetricsStorage;
using MetricsStoragePtr = std::shared_ptr<MetricsStorage>;

}  // namespace utils::statistics

USERVER_NAMESPACE_END
