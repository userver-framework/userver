#include <userver/storages/secdist/helpers.hpp>

#include <userver/formats/json/exception.hpp>
#include <userver/storages/secdist/exceptions.hpp>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace storages::secdist {

[[noreturn]] void ThrowInvalidSecdistType(const formats::json::Value& val, std::string_view type) {
    throw InvalidSecdistJson(fmt::format("'{}' is not {} (or not found)", val.GetPath(), type));
}

std::string GetString(const formats::json::Value& parent_val, std::string_view name) {
    const auto& val = parent_val[name];
    if (!val.IsString()) {
        ThrowInvalidSecdistType(val, "a string");
    }
    return val.As<std::string>();
}

int GetInt(const formats::json::Value& parent_val, std::string_view name, int dflt) {
    const auto& val = parent_val[name];
    try {
        return val.As<int>();
    } catch (const formats::json::MemberMissingException&) {
        return dflt;
    } catch (const formats::json::TypeMismatchException&) {
        ThrowInvalidSecdistType(val, "an int");
    }
}

void CheckIsObject(const formats::json::Value& val, std::string_view) {
    if (!val.IsObject()) {
        ThrowInvalidSecdistType(val, "an object");
    }
}

void CheckIsArray(const formats::json::Value& val, std::string_view) {
    if (!val.IsArray()) {
        ThrowInvalidSecdistType(val, "an array");
    }
}

}  // namespace storages::secdist

USERVER_NAMESPACE_END
