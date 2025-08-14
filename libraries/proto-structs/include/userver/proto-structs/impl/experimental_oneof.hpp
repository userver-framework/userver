#pragma once

#include <cstddef>
#include <optional>
#include <utility>
#include <variant>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::impl {

template <typename... FieldTypes>
class OneofBase {
public:
    OneofBase() = default;

    explicit operator bool() const noexcept { return storage_.has_value(); }

protected:
    template <std::size_t Index>
    bool Has() const noexcept {
        return storage_ && storage_->index() == Index;
    }

    template <std::size_t Index>
    const auto& Get() const {
        return std::get<Index>(storage_.value());
    }

    template <std::size_t Index, typename... Args>
    void Emplace(Args&&... args) {
        storage_.emplace(std::in_place_index<Index>, std::forward<Args>(args)...);
    }

private:
    std::optional<std::variant<FieldTypes...>> storage_;
};

}  // namespace proto_structs::impl

USERVER_NAMESPACE_END

#define UPROTO_ONEOF_HEADER(oneof_type)                                                   \
private:                                                                                  \
    enum { kCounterStart = __COUNTER__ + 1 }; /* An inline constant would violate odr. */ \
public:

#define UPROTO_ONEOF_FIELD(field_type, field_name)                                     \
private:                                                                               \
    static constexpr std::size_t field_name##_index = __COUNTER__ - kCounterStart;     \
                                                                                       \
public:                                                                                \
    bool has_##field_name() const noexcept { return this->Has<field_name##_index>(); } \
    const field_type& field_name() const { return this->Get<field_name##_index>(); }   \
    void set_##field_name(field_type&& value) { this->Emplace<field_name##_index>(std::move(value)); }
