#include <etcd/etcd_responses.hpp>

#include <userver/etcd/exceptions.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::parse {

etcd::EtcdWatchResponse Parse(const formats::json::Value& value, To<etcd::EtcdWatchResponse>) {
    if (!value.HasMember("result")) {
        throw etcd::EtcdWatchResponseParseError(
            fmt::format("No result in watch response: {}", formats::json::ToString(value))
        );
    }
    if (!value["result"].HasMember("events")) {
        throw etcd::EtcdWatchResponseParseError(
            fmt::format("No events in watch response: {}", formats::json::ToString(value))
        );
    }
    etcd::EtcdWatchResponse etcd_watch_response;
    for (const auto& event : value["result"]["events"]) {
        if (!event.HasMember("kv")) {
            LOG_DEBUG() << "Event is not key value change, skipping";
            continue;
        }
        etcd_watch_response.raw_key_value_states.push_back(event["kv"].As<etcd_schemas::RawKeyValueState>());
    }
    return etcd_watch_response;
}

}  // namespace formats::parse

USERVER_NAMESPACE_END
