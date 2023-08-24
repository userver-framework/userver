#pragma once

#include <optional>
#include <string>

USERVER_NAMESPACE_BEGIN

namespace congestion_control {

struct Limit {
  std::optional<size_t> load_limit;
  size_t current_load{0};

  std::string ToLogString() {
    return "limit=" +
           (load_limit ? std::to_string(*load_limit) : std::string("(none)"));
  }
};

class Limiter {
 public:
  virtual void SetLimit(const Limit& new_limit) = 0;

 protected:
  ~Limiter() = default;
};

}  // namespace congestion_control

USERVER_NAMESPACE_END
