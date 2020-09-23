/**
        curl-ev: wrapper for integrating libcurl with libev applications
        Copyright (c) 2013 Oliver Kuckertz <oliver.kuckertz@mologie.de>
        See COPYING for license information.

        Integration of libcurl's error codes (CURLcode) into boost.system's
   error_code class
*/

#include <curl-ev/error_code.hpp>

namespace curl::errc {

class EasyErrorCategory final : public std::error_category {
 public:
  using std::error_category::error_category;

  const char* name() const noexcept override { return "curl-easy"; }

  std::string message(int ev) const override {
    return native::curl_easy_strerror(static_cast<native::CURLcode>(ev));
  }
};

class MultiErrorCategory final : public std::error_category {
 public:
  using std::error_category::error_category;

  const char* name() const noexcept override { return "curl-multi"; }

  std::string message(int ev) const override {
    return native::curl_multi_strerror(static_cast<native::CURLMcode>(ev));
  }
};

class ShareErrorCategory final : public std::error_category {
 public:
  using std::error_category::error_category;

  const char* name() const noexcept override { return "curl-share"; }

  std::string message(int ev) const override {
    return native::curl_share_strerror(static_cast<native::CURLSHcode>(ev));
  }
};

class FormErrorCategory final : public std::error_category {
 public:
  using std::error_category::error_category;

  const char* name() const noexcept override { return "curl-form"; }

  std::string message(int ev) const override {
    switch (static_cast<FormErrorCode>(ev)) {
      case FormErrorCode::kSuccess:
        return "no error";
      case FormErrorCode::kMemory:
        return "memory allocation error";
      case FormErrorCode::kOptionTwice:
        return "one option is given twice";
      case FormErrorCode::kNull:
        return "a null pointer was given for a char";
      case FormErrorCode::kUnknownOption:
        return "an unknown option was used";
      case FormErrorCode::kIncomplete:
        return "some FormInfo is not complete (or error)";
      case FormErrorCode::kIllegalArray:
        return "an illegal option is used in an array";
      case FormErrorCode::kDisabled:
        return "form support was disabled";
      default:
        return "unknown CURLFORMcode";
    }
  }
};

const std::error_category& GetEasyCategory() noexcept {
  static const EasyErrorCategory kEasyCategory;
  return kEasyCategory;
}

const std::error_category& GetMultiCategory() noexcept {
  static const MultiErrorCategory kMultiCategory;
  return kMultiCategory;
}

const std::error_category& GetShareCategory() noexcept {
  static const ShareErrorCategory kShareCategory;
  return kShareCategory;
}

const std::error_category& GetFormCategory() noexcept {
  static const FormErrorCategory kFormCategory;
  return kFormCategory;
}

}  // namespace curl::errc
