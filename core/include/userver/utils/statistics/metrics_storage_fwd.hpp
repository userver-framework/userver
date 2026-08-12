#pragma once

/// @file userver/utils/statistics/metrics_storage_fwd.hpp
/// @brief Forward declarations for utils::statistics::MetricsStorage

#include <memory>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

class MetricsStorage;

using MetricsStoragePtr = std::shared_ptr<MetricsStorage>;

}  // namespace utils::statistics

USERVER_NAMESPACE_END
