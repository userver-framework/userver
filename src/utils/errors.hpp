#pragma once
#include <userver/formats/json/value_builder.hpp>
#include <userver/server/handlers/exceptions.hpp>
#include "make_error.hpp"

namespace real_medium::utils::error {

class ErrorBuilder {
 public:
  static constexpr bool kIsExternalBodyFormatted{true};

  ErrorBuilder(std::string_view field, std::string_view msg);
  std::string GetExternalBody() const;

 private:
  std::string json_error_body_;
};

class ValidationException
    : public userver::server::handlers::ExceptionWithCode<
          userver::server::handlers::HandlerErrorCode::kClientError> {
 public:
  using BaseType::BaseType;

  userver::formats::json::Value ToJson() const;
};

class SlugifyException
    : public userver::server::handlers::ExceptionWithCode<
          userver::server::handlers::HandlerErrorCode::kClientError> {
 public:
  using BaseType::BaseType;

  userver::formats::json::Value ToJson() const;
};


}  // namespace real_medium::utils::error
