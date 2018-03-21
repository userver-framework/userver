#pragma once

#include <string>

#include "request_base.hpp"

namespace server {
namespace request {

class Monitor {
 public:
  Monitor() = default;
  virtual ~Monitor() {}

  virtual std::string GetJsonData(const RequestBase&) const = 0;
};

}  // namespace request
}  // namespace server
