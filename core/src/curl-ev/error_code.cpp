/**
        curl-ev: wrapper for integrating libcurl with libev applications
        Copyright (c) 2013 Oliver Kuckertz <oliver.kuckertz@mologie.de>
        See COPYING for license information.

        Integration of libcurl's error codes (CURLcode) into boost.system's
   error_code class
*/

#include <curl-ev/error_code.hpp>

USERVER_NAMESPACE_BEGIN

namespace curl::errc {
namespace {

class EasyErrorCategory final : public std::error_category {
 public:
  using std::error_category::error_category;

  [[nodiscard]] const char* name() const noexcept override {
    return "curl-easy";
  }

  [[nodiscard]] std::string message(int cond) const override {
    return native::curl_easy_strerror(static_cast<native::CURLcode>(cond));
  }
};

class MultiErrorCategory final : public std::error_category {
 public:
  using std::error_category::error_category;

  [[nodiscard]] const char* name() const noexcept override {
    return "curl-multi";
  }

  [[nodiscard]] std::string message(int cond) const override {
    return native::curl_multi_strerror(static_cast<native::CURLMcode>(cond));
  }
};

class ShareErrorCategory final : public std::error_category {
 public:
  using std::error_category::error_category;

  [[nodiscard]] const char* name() const noexcept override {
    return "curl-share";
  }

  [[nodiscard]] std::string message(int cond) const override {
    return native::curl_share_strerror(static_cast<native::CURLSHcode>(cond));
  }
};

class FormErrorCategory final : public std::error_category {
 public:
  using std::error_category::error_category;

  [[nodiscard]] const char* name() const noexcept override {
    return "curl-form";
  }

  [[nodiscard]] std::string message(int cond) const override {
    switch (static_cast<FormErrorCode>(cond)) {
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

class UrlErrorCategory final : public std::error_category {
 public:
  using std::error_category::error_category;

  [[nodiscard]] const char* name() const noexcept override {
    return "curl-url";
  }

  [[nodiscard]] std::string message(int cond) const override {
    switch (static_cast<UrlErrorCode>(cond)) {
      case UrlErrorCode::kSuccess:
        return "no error";
      case UrlErrorCode::kBadHandle:
        return "a null pointer was given for a URL handle";
      case UrlErrorCode::kBadPartpointer:
        return "a null pointer war given as a 'part' argument";
      case UrlErrorCode::kMalformedInput:
        return "malformed input";
      case UrlErrorCode::kBadPortNumber:
        return "invalid port number";
      case UrlErrorCode::kUnsupportedScheme:
        return "this build of curl does not support the requested scheme";
      case UrlErrorCode::kUrldecode:
        return "URL decoding error";
      case UrlErrorCode::kOutOfMemory:
        return "memory allocation failed";
      case UrlErrorCode::kUserNotAllowed:
        return "credentials are not allowed in the URL";
      case UrlErrorCode::kUnknownPart:
        return "unknown URL part requested";
      case UrlErrorCode::kNoScheme:
        return "URL scheme was not specified";
      case UrlErrorCode::kNoUser:
        return "URL does not contain user part";
      case UrlErrorCode::kNoPassword:
        return "URL does not contain password part";
      case UrlErrorCode::kNoOptions:
        return "URL does not contain options part";
      case UrlErrorCode::kNoHost:
        return "URL does not contain host";
      case UrlErrorCode::kNoPort:
        return "URL does not contain port";
      case UrlErrorCode::kNoQuery:
        return "URL does not contain query part";
      case UrlErrorCode::kNoFragment:
        return "URL does not contain fragment part";
    }

    return "Unknown URL error";
  }
};

class RateLimitErrorCategory final : public std::error_category {
 public:
  using std::error_category::error_category;

  [[nodiscard]] const char* name() const noexcept override {
    return "userver-curl-ratelimit";
  }

  [[nodiscard]] std::string message(int cond) const override {
    switch (static_cast<RateLimitErrorCode>(cond)) {
      case RateLimitErrorCode::kSuccess:
        return "no error";
      case RateLimitErrorCode::kGlobalSocketLimit:
        return "hit global opensocket rate limit";
      case RateLimitErrorCode::kPerHostSocketLimit:
        return "hit per-host opensocket rate limit";
    }

    return "Unknown rate-limit error";
  }
};

}  // namespace

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

const std::error_category& GetUrlCategory() noexcept {
  static const UrlErrorCategory kUrlCategory;
  return kUrlCategory;
}

const std::error_category& GetRateLimitCategory() noexcept {
  static const RateLimitErrorCategory kRateLimitCategory;
  return kRateLimitCategory;
}

}  // namespace curl::errc

USERVER_NAMESPACE_END
