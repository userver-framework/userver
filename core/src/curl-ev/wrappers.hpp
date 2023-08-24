#pragma once

/// @file curl-ev/wrappers.hpp
/// @brief Convenience and memory management wrappers for cURL

#include <memory>

#include "native.hpp"

USERVER_NAMESPACE_BEGIN

namespace curl::impl {

class CurlGlobal final {
 public:
  static void Init();

 private:
  CurlGlobal();
  ~CurlGlobal();
};

struct CurlDeleter final {
  void operator()(char* ptr) const noexcept { native::curl_free(ptr); }
};
using CurlPtr = std::unique_ptr<char, CurlDeleter>;

struct UrlDeleter final {
  void operator()(native::CURLU* url) const noexcept {
    native::curl_url_cleanup(url);
  }
};
using UrlPtr = std::unique_ptr<native::CURLU, UrlDeleter>;

}  // namespace curl::impl

USERVER_NAMESPACE_END
