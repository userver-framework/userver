/**
        curl-ev: wrapper for integrating libcurl with libev applications
        Copyright (c) 2013 Oliver Kuckertz <oliver.kuckertz@mologie.de>
        See COPYING for license information.

        C++ wrapper for libcurl's string list interface
*/

#include <curl-ev/string_list.hpp>

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace curl;

string_list::string_list() : list_(nullptr) {
  initref_ = initialization::ensure_initialization();
}

string_list::~string_list() { clear(); }

void string_list::add(const char* str) {
  native::curl_slist* p = native::curl_slist_append(list_, str);

  if (!p) {
    throw std::bad_alloc();
  }

  list_ = p;
}

void string_list::add(const std::string& str) { add(str.c_str()); }

void string_list::clear() noexcept {
  if (list_) {
    native::curl_slist_free_all(list_);
    list_ = nullptr;
  }
}
