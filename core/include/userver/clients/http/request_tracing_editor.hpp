#pragma once

/// @file userver/clients/http/request_tracing_editor.hpp
/// @brief @copybrief clients::http::RequestTracingEditor

#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace curl {
class easy;
}  // namespace curl

namespace clients::http {

class RequestState;

/// @brief Auxiliary entity that allows editing request to a client
/// from plugins
class RequestTracingEditor final {
 public:
  /// sets header
  void SetHeader(std::string_view, std::string_view);

 private:
  friend class RequestState;

  explicit RequestTracingEditor(curl::easy&);

  curl::easy& easy_;
};

}  // namespace clients::http

USERVER_NAMESPACE_END
