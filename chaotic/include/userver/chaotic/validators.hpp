#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

#include <fmt/format.h>

#include <userver/chaotic/exception.hpp>
#include <userver/utils/text_light.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic {

template <const auto& Value>
struct Minimum final {
    template <typename T>
    static void Validate(T value) {
        static_assert(std::is_arithmetic_v<T>);

        if (value < Value) throw std::runtime_error(fmt::format("Invalid value, minimum={}, given={}", Value, value));
    }
};

template <const auto& Value>
struct Maximum final {
    template <typename T>
    static void Validate(T value) {
        static_assert(std::is_arithmetic_v<T>);

        if (value > Value) throw std::runtime_error(fmt::format("Invalid value, maximum={}, given={}", Value, value));
    }
};

template <const auto& Value>
struct ExclusiveMinimum final {
    template <typename T>
    static void Validate(T value) {
        static_assert(std::is_arithmetic_v<T>);

        if (value <= Value)
            throw std::runtime_error(fmt::format("Invalid value, exclusive minimum={}, given={}", Value, value));
    }
};

template <const auto& Value>
struct ExclusiveMaximum final {
    template <typename T>
    static void Validate(T value) {
        static_assert(std::is_arithmetic_v<T>);

        if (value >= Value)
            throw std::runtime_error(fmt::format("Invalid value, exclusive maximum={}, given={}", Value, value));
    }
};

template <std::int64_t Value>
struct MinItems final {
    template <typename T>
    static void Validate(const T& value) {
        if (value.size() < Value) {
            throw std::runtime_error(fmt::format("Too short array, minimum length={}, given={}", Value, value.size()));
        }
    }
};

template <std::int64_t Value>
struct MaxItems final {
    template <typename T>
    static void Validate(const T& value) {
        if (value.size() > Value) {
            throw std::runtime_error(fmt::format("Too long array, maximum length={}, given={}", Value, value.size()));
        }
    }
};

template <std::int64_t Value>
struct MinLength final {
    static void Validate(std::string_view value) {
        auto length = utils::text::utf8::GetCodePointsCount(value);
        if (length < Value) {
            throw std::runtime_error(fmt::format("Too short string, minimum length={}, given={}", Value, length));
        }
    }
};

template <std::int64_t Value>
struct MaxLength final {
    static void Validate(std::string_view value) {
        auto length = utils::text::utf8::GetCodePointsCount(value);
        if (length > Value) {
            throw std::runtime_error(fmt::format("Too long string, maximum length={}, given={}", Value, length));
        }
    }
};

template <typename... Validators, typename Obj, typename Value>
void Validate(const Obj& obj, const Value& value) {
    try {
        (Validators::Validate(obj), ...);
    } catch (const std::exception& e) {
        chaotic::ThrowForValue(e.what(), value);
    }
}

}  // namespace chaotic

USERVER_NAMESPACE_END
