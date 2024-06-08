#include "view.hpp"

#include <boost/range/adaptor/transformed.hpp>

#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/serialize/common_containers.hpp>

#include <userver/ydb/table.hpp>

namespace sample {

formats::json::Value DescribeTableHandler::HandleRequestJsonThrow(
    const server::http::HttpRequest&, const formats::json::Value& request_json,
    server::request::RequestContext&) const {
  auto response = Ydb().DescribeTable(request_json["path"].As<std::string>());
  const auto& key_columns =
      response.GetTableDescription().GetPrimaryKeyColumns();

  formats::json::ValueBuilder response_builder(formats::json::Type::kObject);
  response_builder["key_columns"] =
      key_columns | boost::adaptors::transformed(
                        [](const auto& string) { return std::string{string}; });
  return response_builder.ExtractValue();
}

}  // namespace sample
