/**
        curl-ev: wrapper for integrating libcurl with libev applications
        Copyright (c) 2013 Oliver Kuckertz <oliver.kuckertz@mologie.de>
        See COPYING for license information.

        Integration of libcurl's error codes (CURLcode) into boost.system's
   error_code class
*/

#include "error_code.hpp"

using namespace curl;

class curl_easy_error_category : public std::error_category {
 public:
  curl_easy_error_category() {}
  const char* name() const noexcept override;
  std::string message(int ev) const override;
};

const char* curl_easy_error_category::name() const noexcept {
  return "curl-easy";
}

std::string curl_easy_error_category::message(int ev) const {
  return native::curl_easy_strerror(static_cast<native::CURLcode>(ev));
}

class curl_multi_error_category : public std::error_category {
 public:
  curl_multi_error_category() {}
  const char* name() const noexcept override;
  std::string message(int ev) const override;
};

const char* curl_multi_error_category::name() const noexcept {
  return "curl-multi";
}

std::string curl_multi_error_category::message(int ev) const {
  return native::curl_multi_strerror(static_cast<native::CURLMcode>(ev));
}

class curl_share_error_category : public std::error_category {
 public:
  curl_share_error_category() {}
  const char* name() const noexcept override;
  std::string message(int ev) const override;
};

const char* curl_share_error_category::name() const noexcept {
  return "curl-share";
}

std::string curl_share_error_category::message(int ev) const {
  return native::curl_share_strerror(static_cast<native::CURLSHcode>(ev));
}

class curl_form_error_category : public std::error_category {
 public:
  curl_form_error_category() {}
  const char* name() const noexcept override;
  std::string message(int ev) const override;
};

const char* curl_form_error_category::name() const noexcept {
  return "curl-form";
}

std::string curl_form_error_category::message(int ev) const {
  switch (static_cast<errc::form::form_error_codes>(ev)) {
    case errc::form::success:
      return "no error";

    case errc::form::memory:
      return "memory allocation error";

    case errc::form::option_twice:
      return "one option is given twice";

    case errc::form::null:
      return "a null pointer was given for a char";

    case errc::form::unknown_option:
      return "an unknown option was used";

    case errc::form::incomplete:
      return "some FormInfo is not complete (or error)";

    case errc::form::illegal_array:
      return "an illegal option is used in an array";

    case errc::form::disabled:
      return "form support was disabled";

    default:
      return "no error description (unknown CURLFORMcode)";
  }
}

const std::error_category& errc::get_easy_category() noexcept {
  static const curl_easy_error_category curl_easy_category_const;
  return curl_easy_category_const;
}

const std::error_category& errc::get_multi_category() noexcept {
  static const curl_multi_error_category curl_multi_category_const;
  return curl_multi_category_const;
}

const std::error_category& errc::get_share_category() noexcept {
  static const curl_share_error_category curl_share_category_const;
  return curl_share_category_const;
}

const std::error_category& errc::get_form_category() noexcept {
  static const curl_form_error_category curl_form_category_const;
  return curl_form_category_const;
}
