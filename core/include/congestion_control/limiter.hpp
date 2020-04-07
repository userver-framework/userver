#pragma once

#include <optional>

namespace congestion_control {

struct Limit {
  std::optional<size_t> load_limit;
};

class Limiter {
 public:
  virtual void SetLimit(const Limit& new_limit) = 0;
};

}  // namespace congestion_control
