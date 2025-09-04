#include <userver/ugrpc/client/qos.hpp>

#include <fmt/format.h>

#include <boost/pfr/ops_fields.hpp>

#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/serialize_duration.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

bool operator==(const Qos& lhs, const Qos& rhs) noexcept { return boost::pfr::eq_fields(lhs, rhs); }

Qos Parse(const formats::json::Value& value, formats::parse::To<Qos>) {
    Qos result;

    const auto attempts = value["attempts"].As<std::optional<int>>();
    if (attempts.has_value()) {
        if (*attempts < 1) {
            throw formats::json::ParseException(
                fmt::format("Invalid value: 'attempts' current value is {}, should be minimum '1'", *attempts)
            );
        }
        result.attempts = attempts;
    }

    const auto timeout_ms = value["timeout-ms"].As<std::optional<std::chrono::milliseconds::rep>>();
    if (timeout_ms) {
        result.timeout = std::chrono::milliseconds{*timeout_ms};
    }

    return result;
}

formats::json::Value Serialize(const Qos& qos, formats::serialize::To<formats::json::Value>) {
    formats::json::ValueBuilder result{formats::common::Type::kObject};

    result["attempts"] = qos.attempts;

    result["timeout-ms"] = qos.timeout;

    return result.ExtractValue();
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
