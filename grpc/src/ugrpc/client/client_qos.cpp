#include <userver/ugrpc/client/client_qos.hpp>

#include <boost/pfr/ops_fields.hpp>

#include <userver/formats/json/inline.hpp>
#include <userver/formats/json/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

bool operator==(const GlobalQos& lhs, const GlobalQos& rhs) noexcept { return boost::pfr::eq_fields(lhs, rhs); }

GlobalQos Parse(const formats::json::Value&, formats::parse::To<GlobalQos>) { return GlobalQos{}; }

formats::json::Value Serialize(const GlobalQos&, formats::serialize::To<formats::json::Value>) {
    return formats::json::MakeObject();
}

bool operator==(const ClientQos& lhs, const ClientQos& rhs) noexcept { return boost::pfr::eq_fields(lhs, rhs); }

ClientQos Parse(const formats::json::Value& value, formats::parse::To<ClientQos>) {
    return ClientQos{value["methods"].As<utils::DefaultDict<Qos>>(), value["global"].As<std::optional<GlobalQos>>()};
}

formats::json::Value Serialize(const ClientQos& client_qos, formats::serialize::To<formats::json::Value>) {
    formats::json::ValueBuilder builder;
    builder["methods"] = client_qos.methods;
    if (client_qos.global.has_value()) {
        builder["global"] = *client_qos.global;
    }
    return builder.ExtractValue();
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
