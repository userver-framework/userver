#pragma once

#include <userver/clients/http/form.hpp>
#include <userver/clients/http/request.hpp>
#include <userver/formats/json.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/http/content_type.hpp>

#include <userver/utils/assert.hpp>

#include <userver/logging/log.hpp>

#include <variant>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

template <typename Method>
void FillRequestDataAsJson(clients::http::Request& request,
                           const typename Method::Parameters& parameters) {
  clients::http::Headers headers;
  headers[http::headers::kContentType] =
    http::content_type::kApplicationJson.ToString();
  
  std::string data = formats::json::ToString(
        formats::json::ValueBuilder(parameters).ExtractValue());

  request.headers(headers).data(std::move(data));
}

template <typename T>
void FillFormSection(clients::http::Form& form,
                     const std::string& field_name,
                     const T& field) {
  static_assert(!std::is_same_v<T, std::string>);
  static_assert(!std::is_same_v<T, bool>);

  std::string content = formats::json::ToString(
        formats::json::ValueBuilder(field).ExtractValue());

  LOG_INFO() << "field_name: " << field_name << "as json";

  form.AddContent(field_name,
                  content,
                  http::content_type::kApplicationJson.ToString());
}

void FillFormSection(clients::http::Form& form,
                     const std::string& field_name,
                     const std::string& field);

void FillFormSection(clients::http::Form& form,
                     const std::string& field_name,
                     bool field);

template <typename T>
void FillFormSection(clients::http::Form& form,
                     const std::string& field_name,
                     const std::optional<T>& field) {
  if (field) {
    FillFormSection(form, field_name, field.value());
  }
}

template <typename T>
void FillFormSection(clients::http::Form& form,
                     const std::string& field_name,
                     const std::unique_ptr<T>& field) {
  if (field) {
    FillFormSection(form, field_name, *field);
  }
}

template <typename... Types>
void FillFormSection(clients::http::Form& form,
                     const std::string& field_name,
                     const std::variant<Types...>& field) {
  std::visit(
    [&form, field_name](const auto& item) {
      return FillFormSection(form, field_name, item);
    },
    field);
}

template <typename Method>
typename Method::Reply ParseResponseDataFromJson(
    const clients::http::Response& response) {
  auto json_response = formats::json::FromString(response.body_view());
  if (!json_response["ok"].As<bool>()) {
    int error_code = json_response["error_code"].As<int>();
    std::string description = json_response["description"].As<std::string>();
    if (400 <= error_code && error_code < 500) {
      throw clients::http::HttpClientException(error_code,
                                               response.GetStats(),
                                               description);
    } else if (500 <= error_code && error_code < 600) {
      throw clients::http::HttpServerException(error_code,
                                               response.GetStats(),
                                               description);
    }
    UINVARIANT(false, fmt::format("Unexpected error: {}, description: {}",
                                  error_code,
                                  description));
  }
  response.raise_for_status();
  return json_response["result"].As<typename Method::Reply>();
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
