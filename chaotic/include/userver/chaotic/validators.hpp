#pragma once

#include <cstdint>
#include <string>

#include <userver/chaotic/exception.hpp>
#include <userver/utils/text_light.hpp>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace chaotic {

template <const auto& Value>
struct Minimum final {
    template <typename T, typename ErrorReporter>
    static void Validate(T value, ErrorReporter report_error) {
        static_assert(std::is_arithmetic_v<T>);

        if (value < Value) {
            report_error(fmt::format("Invalid value, minimum={}, given={}", Value, value));
        }
    }
};

template <const auto& Value>
struct Maximum final {
    template <typename T, typename ErrorReporter>
    static void Validate(T value, ErrorReporter report_error) {
        static_assert(std::is_arithmetic_v<T>);

        if (value > Value) {
            report_error(fmt::format("Invalid value, maximum={}, given={}", Value, value));
        }
    }
};

template <const auto& Value>
struct ExclusiveMinimum final {
    template <typename T, typename ErrorReporter>
    static void Validate(T value, ErrorReporter report_error) {
        static_assert(std::is_arithmetic_v<T>);

        if (value <= Value) {
            report_error(fmt::format("Invalid value, exclusive minimum={}, given={}", Value, value));
        }
    }
};

template <const auto& Value>
struct ExclusiveMaximum final {
    template <typename T, typename ErrorReporter>
    static void Validate(T value, ErrorReporter report_error) {
        static_assert(std::is_arithmetic_v<T>);

        if (value >= Value) {
            report_error(fmt::format("Invalid value, exclusive maximum={}, given={}", Value, value));
        }
    }
};

template <std::int64_t Value>
struct MinItems final {
    template <typename T, typename ErrorReporter>
    static void Validate(const T& value, ErrorReporter report_error) {
        if (value.size() < Value) {
            report_error(fmt::format("Too short array, minimum length={}, given={}", Value, value.size()));
        }
    }
};

template <std::int64_t Value>
struct MaxItems final {
    template <typename T, typename ErrorReporter>
    static void Validate(const T& value, ErrorReporter report_error) {
        if (value.size() > Value) {
            report_error(fmt::format("Too long array, maximum length={}, given={}", Value, value.size()));
        }
    }
};

template <std::int64_t Value>
struct MinLength final {
    template <typename ErrorReporter>
    static void Validate(std::string_view value, ErrorReporter report_error) {
        const auto length = utils::text::utf8::GetCodePointsCount(value);
        if (length < Value) {
            report_error(fmt::format("Too short string, minimum length={}, given={}", Value, length));
        }
    }
};

template <std::int64_t Value>
struct MaxLength final {
    template <typename ErrorReporter>
    static void Validate(std::string_view value, ErrorReporter report_error) {
        const auto length = utils::text::utf8::GetCodePointsCount(value);
        if (length > Value) {
            report_error(fmt::format("Too long string, maximum length={}, given={}", Value, length));
        }
    }
};

template <typename... Validators, typename Obj, typename Value>
void Validate(const Obj& obj, const Value& value) {
    [[maybe_unused]] const auto error_reporter = [&value](std::string_view error) {
        chaotic::ThrowForValue(error, value);
    };
    (Validators::Validate(obj, error_reporter), ...);
}

}  // namespace chaotic

USERVER_NAMESPACE_END
