/**
        curl-ev: wrapper for integrating libcurl with libev applications
        Copyright (c) 2013 Oliver Kuckertz <oliver.kuckertz@mologie.de>
        See COPYING for license information.

        C++ wrapper for libcurl's share interface
*/

#pragma once

#include <mutex>

#include "config.hpp"
#include "initialization.hpp"
#include "native.hpp"

namespace curl {
class CURLASIO_API share : public std::enable_shared_from_this<share> {
 public:
  share();
  share(const share&) = delete;
  ~share();

  inline native::CURLSH* native_handle() { return handle_; }
  void set_share_cookies(bool enabled);
  void set_share_dns(bool enabled);
  void set_share_ssl_session(bool enabled);

  typedef void (*lock_function_t)(native::CURL* handle,
                                  native::curl_lock_data data,
                                  native::curl_lock_access access,
                                  void* userptr);
  void set_lock_function(lock_function_t lock_function);

  typedef void (*unlock_function_t)(native::CURL* handle,
                                    native::curl_lock_data data, void* userptr);
  void set_unlock_function(unlock_function_t unlock_function);

  void set_user_data(void* user_data);

 private:
  static void lock(native::CURL* handle, native::curl_lock_data data,
                   native::curl_lock_access access, void* userptr);
  static void unlock(native::CURL* handle, native::curl_lock_data data,
                     void* userptr);

  initialization::ptr initref_;
  native::CURLSH* handle_;
  std::mutex mutex_;
};
}  // namespace curl
