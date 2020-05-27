/** @file curl-ev/string_list.hpp
        curl-ev: wrapper for integrating libcurl with libev applications
        Copyright (c) 2013 Oliver Kuckertz <oliver.kuckertz@mologie.de>
        See COPYING for license information.

        Constructs libcurl string lists
*/

#pragma once

#include <string>

#include "config.hpp"
#include "initialization.hpp"
#include "native.hpp"

namespace curl {

class CURLASIO_API string_list
    : public std::enable_shared_from_this<string_list> {
 public:
  string_list();
  string_list(const string_list&) = delete;
  ~string_list();

  inline native::curl_slist* native_handle() { return list_; }

  void add(const char* str);
  void add(const std::string& str);
  void clear() noexcept;

 private:
  initialization::ptr initref_;
  native::curl_slist* list_{nullptr};
};
}  // namespace curl
