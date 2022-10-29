#pragma once

/// @file userver/utils/statistics/fwd.hpp
/// @brief Forward declarations for statistics entities

#include <memory>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

class Storage;
class Entry;
class StatisticsRequest;
class Writer;

class MetricsStorage;
using MetricsStoragePtr = std::shared_ptr<MetricsStorage>;

}  // namespace utils::statistics

USERVER_NAMESPACE_END
