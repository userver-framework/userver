/**
        curl-ev: wrapper for integrating libcurl with libev applications
        Copyright (c) 2013 Oliver Kuckertz <oliver.kuckertz@mologie.de>
        See COPYING for license information.

        C++ wrapper for libcurl's share interface
*/

#include <curl-ev/error_code.hpp>
#include <curl-ev/share.hpp>
#include <curl-ev/wrappers.hpp>

USERVER_NAMESPACE_BEGIN

namespace curl {

share::share() {
  impl::CurlGlobal::Init();
  // NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
  handle_ = native::curl_share_init();

  if (!handle_) {
    throw std::bad_alloc();
  }

  set_lock_function(&share::lock);
  set_unlock_function(&share::unlock);
  set_user_data(this);
}

share::~share() {
  if (handle_) {
    native::curl_share_cleanup(handle_);
    handle_ = nullptr;
  }
}

void share::set_share_cookies(bool enabled) {
  std::error_code ec{
      static_cast<errc::ShareErrorCode>(native::curl_share_setopt(
          handle_,
          enabled ? native::CURLSHOPT_SHARE : native::CURLSHOPT_UNSHARE,
          native::CURL_LOCK_DATA_COOKIE))};
  throw_error(ec, __func__);
}

void share::set_share_dns(bool enabled) {
  std::error_code ec{
      static_cast<errc::ShareErrorCode>(native::curl_share_setopt(
          handle_,
          enabled ? native::CURLSHOPT_SHARE : native::CURLSHOPT_UNSHARE,
          native::CURL_LOCK_DATA_DNS))};
  throw_error(ec, __func__);
}

void share::set_share_ssl_session(bool enabled) {
  std::error_code ec{
      static_cast<errc::ShareErrorCode>(native::curl_share_setopt(
          handle_,
          enabled ? native::CURLSHOPT_SHARE : native::CURLSHOPT_UNSHARE,
          native::CURL_LOCK_DATA_SSL_SESSION))};
  throw_error(ec, __func__);
}

void share::set_lock_function(lock_function_t lock_function) {
  std::error_code ec{
      static_cast<errc::ShareErrorCode>(native::curl_share_setopt(
          handle_, native::CURLSHOPT_LOCKFUNC, lock_function))};
  throw_error(ec, __func__);
}

void share::set_unlock_function(unlock_function_t unlock_function) {
  std::error_code ec{
      static_cast<errc::ShareErrorCode>(native::curl_share_setopt(
          handle_, native::CURLSHOPT_UNLOCKFUNC, unlock_function))};
  throw_error(ec, __func__);
}

void share::set_user_data(void* user_data) {
  std::error_code ec{
      static_cast<errc::ShareErrorCode>(native::curl_share_setopt(
          handle_, native::CURLSHOPT_USERDATA, user_data))};
  throw_error(ec, __func__);
}

void share::lock(native::CURL*, native::curl_lock_data,
                 native::curl_lock_access, void* userptr) {
  auto* self = static_cast<share*>(userptr);
  self->mutex_.lock();
}

void share::unlock(native::CURL*, native::curl_lock_data, void* userptr) {
  auto* self = static_cast<share*>(userptr);
  self->mutex_.unlock();
}

}  // namespace curl

USERVER_NAMESPACE_END
