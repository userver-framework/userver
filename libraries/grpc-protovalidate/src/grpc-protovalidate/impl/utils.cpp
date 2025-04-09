#include <grpc-protovalidate/impl/utils.hpp>

#include <string>
#include <string_view>

#include <fmt/format.h>

#include <userver/utils/assert.hpp>

namespace {

std::string GetFieldName(const buf::validate::Violation& violation) {
    if (violation.field().elements().empty()) {
        return "<null>";
    }

    fmt::memory_buffer name;

    for (const auto& item : violation.field().elements()) {
        std::string_view field_name = "<null>";

        if (!item.field_name().empty()) {
            field_name = item.field_name();
        }

        switch (item.subscript_case()) {
            case buf::validate::FieldPathElement::kIndex:
                fmt::format_to(std::back_inserter(name), "{}[{}].", field_name, item.index());
                break;
            case buf::validate::FieldPathElement::kBoolKey:
                fmt::format_to(std::back_inserter(name), "{}[{}].", field_name, item.bool_key() ? "true" : "false");
                break;
            case buf::validate::FieldPathElement::kIntKey:
                fmt::format_to(std::back_inserter(name), "{}[{}].", field_name, item.int_key());
                break;
            case buf::validate::FieldPathElement::kUintKey:
                fmt::format_to(std::back_inserter(name), "{}[{}].", field_name, item.uint_key());
                break;
            case buf::validate::FieldPathElement::kStringKey:
                fmt::format_to(std::back_inserter(name), "{}['{}'].", field_name, item.string_key());
                break;
            default:
                fmt::format_to(std::back_inserter(name), "{}.", field_name);
                break;
        }
    }

    name.resize(name.size() - 1);
    return fmt::to_string(name);
}

std::string_view GetConstraintId(const buf::validate::Violation& violation) {
    if (violation.constraint_id().empty()) {
        return "null";
    }

    return violation.constraint_id();
}

std::string_view GetDescription(const buf::validate::Violation& violation) {
    if (violation.message().empty()) {
        return "<null>";
    }

    return violation.message();
}

}  // namespace

namespace buf::validate {

USERVER_NAMESPACE::logging::LogHelper&
operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const Violation& violation) {
    lh << fmt::format(
        "field={}, constraint={}, description='{}'{}",
        GetFieldName(violation),
        GetConstraintId(violation),
        GetDescription(violation),
        violation.for_key() ? " (key error)" : ""
    );

    return lh;
}

}  // namespace buf::validate

USERVER_NAMESPACE_BEGIN

namespace grpc_protovalidate::impl {

std::unique_ptr<buf::validate::ValidatorFactory> CreateProtoValidatorFactory() {
    auto result = buf::validate::ValidatorFactory::New();
    UINVARIANT(result.ok(), "Failed to create validator factory");
    return std::move(result).value();
}

}  // namespace grpc_protovalidate::impl

USERVER_NAMESPACE_END
