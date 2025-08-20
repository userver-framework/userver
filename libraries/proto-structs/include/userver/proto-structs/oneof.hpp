#pragma once

/// @file userver/proto-structs/oneof.hpp
/// @brief Contains type to represent `oneof` protobuf message fields in a struct

#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

#include <userver/proto-structs/exceptions.hpp>
#include <userver/proto-structs/impl/traits_light.hpp>
#include <userver/proto-structs/type_mapping.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs {

namespace traits {

template <typename T>
concept OneofField =
    std::is_same_v<std::remove_cv_t<T>, bool> || std::is_same_v<std::remove_cv_t<T>, std::int32_t> ||
    std::is_same_v<std::remove_cv_t<T>, std::int64_t> || std::is_same_v<std::remove_cv_t<T>, std::uint32_t> ||
    std::is_same_v<std::remove_cv_t<T>, std::uint64_t> || std::is_same_v<std::remove_cv_t<T>, float> ||
    std::is_same_v<std::remove_cv_t<T>, double> || std::is_same_v<std::remove_cv_t<T>, std::string> ||
    std::is_enum_v<T> || ProtoStruct<T>;

}  // namespace traits

template <traits::OneofField... TFields>
class Oneof;

namespace impl {

template <std::size_t Index, typename T>
struct OneofAlternativeTrait;

/// @brief Provides information about @ref proto_structs::Oneof field type at position `Index`
template <std::size_t Index, proto_structs::traits::OneofField... TFields>
struct OneofAlternativeTrait<Index, Oneof<TFields...>> {
    using Type = std::variant_alternative_t<Index, std::variant<TFields...>>;
};

}  // namespace impl

namespace traits {

/// @brief Concept of the @ref proto_structs::Oneof
template <typename T>
concept Oneof = impl::traits::InheritsFromInstantiation<proto_structs::Oneof, T>;

}  // namespace traits

/// @brief Provides information about @ref proto_structs::Oneof field type at position `Index`
template <std::size_t Index, typename T>
using OneofAlternativeTrait = impl::OneofAlternativeTrait<Index, std::remove_cv_t<T>>;

template <std::size_t Index, typename T>
using OneofAlternativeType = OneofAlternativeTrait<Index, T>::Type;

/// @brief Special index value used to indicate `oneof` without any field set
inline constexpr std::size_t kOneofNpos = -1;

/// @brief Wrapper for `oneof` protobuf message fields
/// @tparam TFields `oneof` field types
template <traits::OneofField... TFields>
class Oneof {
public:
    /// @brief Number of fields in the oneof
    static constexpr inline std::size_t kSize = sizeof...(TFields);

    static_assert(kSize > 0, "Oneof should contain at least one field");

    /// @brief Creates oneof without any field set
    constexpr Oneof() = default;

    /// @brief Creates oneof wrapper initializing @a index field
    template <std::size_t Index, typename... TArgs>
    constexpr Oneof(std::in_place_index_t<Index> index, TArgs&&... args)
        : storage_(std::variant<TFields...>(index, std::forward<TArgs>(args)...)) {}

    /// @brief Returns zero-based index of the alternative held by the oneof.
    /// If oneof does not contain any field, returns @ref proto_structs::kOneofNpos .
    [[nodiscard]] constexpr std::size_t GetIndex() const noexcept { return storage_ ? storage_->index() : kOneofNpos; }

    /// @brief Returns `true` of oneof contains @a index field
    [[nodiscard]] constexpr bool Contains(std::size_t index) const noexcept {
        return storage_ && storage_->index() == index;
    }

    /// @brief Returns `true` of oneof contains some field
    [[nodiscard]] constexpr bool ContainsAny() const noexcept { return storage_.has_value(); }

    /// @brief Returns `Index` field
    /// @throws OneofAccessError if `Index` field is not set
    template <std::size_t Index>
    [[nodiscard]] constexpr const OneofAlternativeType<Index, Oneof<TFields...>>& Get() const& {
        if (!Contains(Index)) {
            throw OneofAccessError(Index);
        }

        return std::get<Index>(*storage_);
    }

    /// @brief Returns `Index` field
    /// @throws OneofAccessError if `Index` field is not set
    template <std::size_t Index>
    [[nodiscard]] constexpr OneofAlternativeType<Index, Oneof<TFields...>>& Get() & {
        if (!Contains(Index)) {
            throw OneofAccessError(Index);
        }

        return std::get<Index>(*storage_);
    }

    /// @brief Returns `Index` field
    /// @throws OneofAccessError if `Index` field is not set
    template <std::size_t Index>
    [[nodiscard]] constexpr OneofAlternativeType<Index, Oneof<TFields...>>&& Get() && {
        if (!Contains(Index)) {
            throw OneofAccessError(Index);
        }

        return std::get<Index>(*std::move(storage_));
    }

    /// @brief Initializes `Index` field in-place and returns it
    /// @tparam TArgs arguments to pass to `Index` field constructor
    template <std::size_t Index, typename... TArgs>
    constexpr OneofAlternativeType<Index, Oneof<TFields...>>& Emplace(TArgs&&... args) {
        if (!storage_) {
            storage_ = std::variant<TFields...>(std::in_place_index<Index>, std::forward<TArgs>(args)...);
            return std::get<Index>(*storage_);
        } else {
            return storage_->template emplace<Index>(std::forward<TArgs>(args)...);
        }
    }

    template <std::size_t Index>
    constexpr void Set(const OneofAlternativeType<Index, Oneof<TFields...>>& value) {
        Emplace<Index>(value);
    }

    template <std::size_t Index>
    constexpr void Set(OneofAlternativeType<Index, Oneof<TFields...>>&& value) {
        Emplace<Index>(std::move(value));
    }

    /// @brief Clear field `Index` if it is set
    void Clear(std::size_t index) noexcept {
        if (Contains(index)) {
            ClearOneof();
        }
    }

    /// @brief Clears oneof
    void ClearOneof() noexcept { storage_.reset(); }

    /// @brief Returns `true` if oneof contains some field
    explicit constexpr operator bool() const noexcept { return ContainsAny(); }

private:
    std::optional<std::variant<TFields...>> storage_;
};

}  // namespace proto_structs

USERVER_NAMESPACE_END
