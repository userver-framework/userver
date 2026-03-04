#include "field_to_string.hpp"

#include <cstdint>
#include <string>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl {

std::string FieldToString(const AMQP::Field& field) {
    const auto format = [](const auto value) { return fmt::format("{}", value); };

    // AMQP-CPP field type codes returned by AMQP::Field::typeID():
    // string: 's'/'S', bool: 't', integers: 'b'/'B'/'U'/'u'/'I'/'i'/'L'/'l', float/double: 'f'/'d'.
    switch (field.typeID()) {
        case 'S':
        case 's':
            return static_cast<const std::string&>(field);
        case 't':
            return static_cast<const AMQP::BooleanSet&>(field).value() ? "true" : "false";
        case 'B':
            return format(static_cast<std::uint32_t>(static_cast<std::uint8_t>(field)));
        case 'b':
            return format(static_cast<std::int32_t>(static_cast<std::int8_t>(field)));
        case 'u':
            return format(static_cast<std::uint16_t>(field));
        case 'U':
            return format(static_cast<std::int16_t>(field));
        case 'i':
            return format(static_cast<std::uint32_t>(field));
        case 'I':
            return format(static_cast<std::int32_t>(field));
        case 'l':
        case 'T':
            return format(static_cast<std::uint64_t>(field));
        case 'L':
            return format(static_cast<std::int64_t>(field));
        case 'f':
            return format(static_cast<float>(field));
        case 'd':
            return format(static_cast<double>(field));
        default:
            return std::string(field);
    }
}

}  // namespace urabbitmq::impl

USERVER_NAMESPACE_END
