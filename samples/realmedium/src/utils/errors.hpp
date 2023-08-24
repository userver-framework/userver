#pragma once
#include <userver/formats/json/value_builder.hpp>
#include <userver/server/handlers/exceptions.hpp>
#include "make_error.hpp"

namespace real_medium::utils::error {

/*
 * userver doesn't yet support 422 HTTP error code
 */
class ValidationException
    : public userver::server::handlers::ExceptionWithCode<
          userver::server::handlers::HandlerErrorCode::kClientError> {
 public:
  ValidationException(std::string_view field, std::string_view msg)
      : BaseType(MakeError(field, msg)) {}

  explicit ValidationException(userver::formats::json::Value&& json)
      : BaseType(std::move(json)) {}
};

class SlugifyException
    : public userver::server::handlers::ExceptionWithCode<
          userver::server::handlers::HandlerErrorCode::kClientError> {
 public:
  using BaseType::BaseType;
};

}  // namespace real_medium::utils::error
