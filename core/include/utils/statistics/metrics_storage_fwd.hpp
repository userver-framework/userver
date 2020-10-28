#pragma once

#include <memory>

namespace utils::statistics {

class MetricsStorage;

using MetricsStoragePtr = std::shared_ptr<MetricsStorage>;

}  // namespace utils::statistics
