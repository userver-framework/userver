#include <curl-ev/wrappers.hpp>

#include <crypto/openssl.hpp>
#include <curl-ev/error_code.hpp>

namespace curl::impl {

CurlGlobal::CurlGlobal() {
  crypto::impl::Openssl::Init();
  std::error_code ec{native::curl_global_init(CURL_GLOBAL_DEFAULT)};
  throw_error(ec, "cURL global initalization failed");
}

CurlGlobal::~CurlGlobal() { native::curl_global_cleanup(); }

void CurlGlobal::Init() { [[maybe_unused]] static CurlGlobal global; }

}  // namespace curl::impl
