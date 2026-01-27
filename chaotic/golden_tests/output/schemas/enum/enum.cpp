#include <userver/chaotic/type_bundle_cpp.hpp>

#include "enum.hpp"
#include "enum_parsers.ipp"

namespace ns {

bool operator==(const Enum& lhs, const Enum& rhs) { return lhs.foo == rhs.foo && true; }

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const Enum::Foo& value) {
    return lh << ToString(value);
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const Enum& value) {
    return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value).ExtractValue());
}

Enum::Foo Parse(USERVER_NAMESPACE::formats::json::Value json, USERVER_NAMESPACE::formats::parse::To<Enum::Foo> to) {
    return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

Enum Parse(USERVER_NAMESPACE::formats::json::Value json, USERVER_NAMESPACE::formats::parse::To<Enum> to) {
    return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

Enum::Foo Parse(USERVER_NAMESPACE::formats::yaml::Value json, USERVER_NAMESPACE::formats::parse::To<Enum::Foo> to) {
    return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

Enum Parse(USERVER_NAMESPACE::formats::yaml::Value json, USERVER_NAMESPACE::formats::parse::To<Enum> to) {
    return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

Enum::Foo Parse(USERVER_NAMESPACE::yaml_config::Value json, USERVER_NAMESPACE::formats::parse::To<Enum::Foo> to) {
    return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

Enum Parse(USERVER_NAMESPACE::yaml_config::Value json, USERVER_NAMESPACE::formats::parse::To<Enum> to) {
    return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

Enum::Foo FromString(std::string_view value, USERVER_NAMESPACE::formats::parse::To<Enum::Foo>) {
    const auto result = k__ns__Enum__Foo_Mapping.TryFindBySecond(value);
    if (result.has_value()) {
        return *result;
    }
    throw std::runtime_error(fmt::format("Invalid enum value ({}) for type ::ns::Enum::Foo", value));
}

Enum::Foo Parse(std::string_view value, USERVER_NAMESPACE::formats::parse::To<Enum::Foo> to) {
    return FromString(value, to);
}

USERVER_NAMESPACE::formats::json::Value
Serialize(const Enum::Foo& value, USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>) {
    return USERVER_NAMESPACE::formats::json::ValueBuilder(ToString(value)).ExtractValue();
}

USERVER_NAMESPACE::
    formats::
        json::
            Value
            Serialize([[maybe_unused]] const Enum& value, USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>) {
    USERVER_NAMESPACE::formats::json::ValueBuilder vb = USERVER_NAMESPACE::formats::common::Type::kObject;

    if (value.foo) {
        vb["foo"] = USERVER_NAMESPACE::chaotic::Primitive<::ns::Enum::Foo>{*value.foo};
    }

    return vb.ExtractValue();
}

std::string ToString(Enum::Foo value) {
    const auto result = k__ns__Enum__Foo_Mapping.TryFindByFirst(value);
    if (result.has_value()) {
        return std::string{*result};
    }
    throw std::runtime_error(fmt::format("Invalid enum value: {}", static_cast<int>(value)));
}

}  // namespace ns

fmt::format_context::iterator fmt::formatter<
    ns::Enum::Foo>::format(const ns::Enum::Foo& value, fmt::format_context& ctx) const {
    return fmt::format_to(ctx.out(), "{}", ToString(value));
}
