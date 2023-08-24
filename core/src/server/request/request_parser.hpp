#pragma once

#include <cstddef>

USERVER_NAMESPACE_BEGIN

namespace server::request {

class RequestParser {
 public:
  RequestParser() = default;
  RequestParser(RequestParser&&) = delete;
  RequestParser& operator=(RequestParser&&) = delete;

  virtual ~RequestParser() noexcept = default;

  virtual bool Parse(const char* data, size_t size) = 0;
};

}  // namespace server::request

USERVER_NAMESPACE_END
