#pragma once

#include <cstddef>
#include <utility>

#include <userver/proto-structs/oneof.hpp>

#define UPROTO_ONEOF_HEADER(oneof_type)                                                   \
private:                                                                                  \
    enum { kCounterStart = __COUNTER__ + 1 }; /* An inline constant would violate odr. */ \
public:                                                                                   \
    using Base::Base;

#define UPROTO_ONEOF_FIELD(oneof_type, field_type, field_name)                     \
private:                                                                           \
    static constexpr std::size_t field_name##_index = __COUNTER__ - kCounterStart; \
                                                                                   \
public:                                                                            \
    [[nodiscard]] constexpr bool has_##field_name() const noexcept;                \
    [[nodiscard]] constexpr const field_type& field_name() const&;                 \
    [[nodiscard]] constexpr field_type&& field_name()&&;                           \
    constexpr void set_##field_name(const field_type& value);                      \
    constexpr void set_##field_name(field_type&& value);                           \
    constexpr field_type& mutable_##field_name();                                  \
    [[nodiscard]] static oneof_type make_##field_name(const field_type& value);    \
    [[nodiscard]] static oneof_type make_##field_name(field_type&& value);

#define UPROTO_ONEOF_DEFINE_FIELD_FUNCTIONS(oneof_type, field_type, field_name)                                    \
    [[nodiscard]] constexpr bool oneof_type::has_##field_name() const noexcept {                                   \
        return Base::Contains(field_name##_index);                                                                 \
    }                                                                                                              \
    [[nodiscard]] constexpr const field_type& oneof_type::field_name() const& {                                    \
        return Base::Get<field_name##_index>();                                                                    \
    }                                                                                                              \
    [[nodiscard]] constexpr field_type&& oneof_type::field_name()&& {                                              \
        return std::move(*this).Base::Get<field_name##_index>();                                                   \
    }                                                                                                              \
    constexpr void oneof_type::set_##field_name(const field_type& value) { Base::Set<field_name##_index>(value); } \
    constexpr void oneof_type::set_##field_name(field_type&& value) {                                              \
        Base::Set<field_name##_index>(std::move(value));                                                           \
    }                                                                                                              \
    constexpr field_type& oneof_type::mutable_##field_name() { return Base::GetMutable<field_name##_index>(); }    \
    [[nodiscard]] inline oneof_type oneof_type::make_##field_name(const field_type& value) {                       \
        return oneof_type(std::in_place_index<field_name##_index>, value);                                         \
    }                                                                                                              \
    [[nodiscard]] inline oneof_type oneof_type::make_##field_name(field_type&& value) {                            \
        return oneof_type(std::in_place_index<field_name##_index>, std::move(value));                              \
    }
