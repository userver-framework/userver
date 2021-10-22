#pragma once

#include <cstddef>

USERVER_NAMESPACE_BEGIN

namespace server {
namespace request {

class RequestParser {
 public:
  virtual ~RequestParser() noexcept = default;

  virtual bool Parse(const char* data, size_t size) = 0;
};

}  // namespace request
}  // namespace server

USERVER_NAMESPACE_END
