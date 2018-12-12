/**
        curl-ev: wrapper for integrating libcurl with libev applications
        Copyright (c) 2013 Oliver Kuckertz <oliver.kuckertz@mologie.de>
        See COPYING for license information.

        Helper to automatically initialize and cleanup libcurl resources
*/

#include <memory>
#include <mutex>
#include <string>

#include <curl-ev/initialization.hpp>
#include <curl-ev/native.hpp>

using namespace curl;

namespace {
std::weak_ptr<initialization> helper_instance;
std::mutex helper_lock;
}  // namespace

initialization::ptr initialization::ensure_initialization() {
  ptr result = helper_instance.lock();

  if (!result) {
    std::lock_guard<std::mutex> lock(helper_lock);
    result = helper_instance.lock();

    if (!result) {
      result = std::shared_ptr<initialization>(new initialization());
      helper_instance = result;
    }
  }

  return result;
}

initialization::initialization() {
  native::CURLcode ec = native::curl_global_init(CURL_GLOBAL_DEFAULT);

  if (ec != native::CURLE_OK) {
    throw std::runtime_error("curl_global_init failed with error code " +
                             std::to_string(ec));
  }
}

initialization::~initialization() { native::curl_global_cleanup(); }
