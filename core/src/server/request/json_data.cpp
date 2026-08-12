#include <userver/server/request/json_data.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::request {

namespace {
constexpr std::string_view kRequestDataName = "__request_object_json";
constexpr std::string_view kResponseDataName = "__response_object_json";
}  // namespace

const formats::json::Value* GetRequestJson(const RequestContext& context) {
    return context.GetDataOptional<const formats::json::Value>(kRequestDataName);
}

formats::json::Value& SetRequestJson(RequestContext& context, formats::json::Value request_json) {
    return context.SetData(std::string{kRequestDataName}, std::move(request_json));
}

const formats::json::Value* GetResponseJson(const RequestContext& context) {
    return context.GetDataOptional<const formats::json::Value>(kResponseDataName);
}

formats::json::Value& SetResponseJson(RequestContext& context, formats::json::Value request_json) {
    return context.SetData(std::string{kResponseDataName}, std::move(request_json));
}

}  // namespace server::request

USERVER_NAMESPACE_END
