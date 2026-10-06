#include <userver/server/request/json_data.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::request {

namespace {
const utils::AnyStorageDataTag<StorageContext, formats::json::Value> kRequestJson;
const utils::AnyStorageDataTag<StorageContext, formats::json::Value> kResponseJson;
}  // namespace

const formats::json::Value* GetRequestJson(const RequestContext& context) {
    return context.GetDataOptional(kRequestJson);
}

formats::json::Value& SetRequestJson(RequestContext& context, formats::json::Value request_json) {
    return context.SetData(kRequestJson, std::move(request_json));
}

const formats::json::Value* GetResponseJson(const RequestContext& context) {
    return context.GetDataOptional(kResponseJson);
}

formats::json::Value& SetResponseJson(RequestContext& context, formats::json::Value request_json) {
    return context.SetData(kResponseJson, std::move(request_json));
}

}  // namespace server::request

USERVER_NAMESPACE_END
