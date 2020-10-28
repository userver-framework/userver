/** @file curl-ev/string_list.hpp
        curl-ev: wrapper for integrating libcurl with libev applications
        Copyright (c) 2013 Oliver Kuckertz <oliver.kuckertz@mologie.de>
        See COPYING for license information.

        Constructs libcurl string lists
*/

#pragma once

#include <memory>
#include <string>

#include "native.hpp"

namespace curl {

class string_list {
 public:
  string_list();
  string_list(const string_list&) = delete;
  string_list(string_list&&) = delete;
  string_list& operator=(const string_list&) = delete;
  string_list& operator=(string_list&&) = delete;
  ~string_list();

  inline native::curl_slist* native_handle() { return list_; }

  void add(const char* str);
  void add(const std::string& str);
  void clear() noexcept;

 private:
  native::curl_slist* list_{nullptr};
};

}  // namespace curl
