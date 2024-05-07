#pragma once

#include <views/base_handler.hpp>

namespace sample {

class UpsertRowsHandler final : public BaseHandler {
 public:
  static constexpr std::string_view kName = "handler-upsert-rows";

  using BaseHandler::BaseHandler;

  formats::json::Value HandleRequestJsonThrow(
      const server::http::HttpRequest& request,
      const formats::json::Value& request_json,
      server::request::RequestContext& context) const override;
};

}  // namespace sample
