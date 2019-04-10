#pragma once

#include <cstddef>

namespace server {
namespace request {

class RequestParser {
 public:
  virtual ~RequestParser() noexcept = default;

  virtual bool Parse(const char* data, size_t size) = 0;
};

}  // namespace request
}  // namespace server
