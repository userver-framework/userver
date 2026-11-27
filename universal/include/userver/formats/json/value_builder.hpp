#pragma once

/// @file userver/formats/json/value_builder.hpp
/// @brief @copybrief formats::json::ValueBuilder

#include <chrono>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include <userver/compiler/impl/nodebug.hpp>
#include <userver/formats/common/meta.hpp>
#include <userver/formats/common/transfer_tag.hpp>
#include <userver/formats/json/impl/mutable_value_wrapper.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/utils/meta.hpp>
#include <userver/utils/strong_typedef.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json {

namespace impl {

template <typename T>
concept IsNativeString =
    std::is_convertible_v<const T&, std::string_view> &&
    (std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view> ||
     (!std::is_class_v<T> && !std::is_union_v<T>) || !common::impl::HasSerialize<formats::json::Value, T>);

template <typename T>
concept IsNativeArithmetic =
    meta::IsCharacter<T> || std::is_same_v<T, bool> || meta::IsInteger<T> ||
    std::is_same_v<std::remove_cv_t<T>, float> || std::is_same_v<std::remove_cv_t<T>, double>;

template <typename T>
concept IsNormalizedArithmetic =
    std::is_same_v<T, bool> || std::is_same_v<T, int> || std::is_same_v<T, unsigned int> ||
    std::is_same_v<T, std::int64_t> || std::is_same_v<T, std::uint64_t> || std::is_same_v<T, float> ||
    std::is_same_v<T, double>;

}  // namespace impl

/// @ingroup userver_universal userver_containers userver_formats
///
/// @brief Builder for JSON.
///
/// Class provides methods for building JSON. For read only access to the
/// existing JSON values use formats::json::Value.
///
/// ## Example usage:
///
/// @snippet universal/src/formats/json/value_builder_test.cpp  Sample formats::json::ValueBuilder usage
///
/// ## Customization example:
///
/// @snippet universal/src/formats/json/value_builder_test.cpp  Sample Customization formats::json::ValueBuilder usage
///
/// @see @ref scripts/docs/en/userver/formats.md
class ValueBuilder final {
public:
    struct IterTraits {
        using ValueType = formats::json::ValueBuilder;
        using Reference = formats::json::ValueBuilder&;
        using Pointer = formats::json::ValueBuilder*;
        using ContainerType = impl::MutableValueWrapper;
    };

    using iterator = Iterator<IterTraits>;

    /// Constructs a ValueBuilder that holds kNull
    ValueBuilder() = default;

    /// Constructs a valueBuilder that holds default value for provided `type`.
    ValueBuilder(formats::common::Type type);

    /// @brief Transfers the `ValueBuilder` object
    /// @see formats::common::TransferTag for the transfer semantics
    ValueBuilder(common::TransferTag, ValueBuilder&&) noexcept;

    ValueBuilder(const ValueBuilder& other);
    // NOLINTNEXTLINE(cppcoreguidelines-noexcept-move-operations, performance-noexcept-move-constructor)
    ValueBuilder(ValueBuilder&& other);

    ValueBuilder(const formats::json::Value& other);
    ValueBuilder(formats::json::Value&& other);

    /// @brief Constructs a number or boolean directly.
    template <typename T>
    requires(impl::IsNativeArithmetic<T> && !meta::IsCharacter<T>)
    ValueBuilder(T value)
        : value_(CreateArithmetic(NormalizeArithmetic(value)))
    {}

    /// @brief Constructs a number representing the character's code unit.
    template <typename T>
    requires(impl::IsNativeArithmetic<T> && meta::IsCharacter<T>)
    [[deprecated(
        "Characters will be converted into underlying integer type. Avoid this or wrap with static_cast<int>(...) "
        "explicitly."
    )]]
    ValueBuilder(T value)
        : value_(CreateArithmetic(NormalizeArithmetic(value)))
    {}

    /// @brief Constructs a string, copying its bytes into owned storage.
    template <typename T>
    requires impl::IsNativeString<T>
    ValueBuilder(const T& value)
        : value_(CreateString(std::string_view{value}))
    {}
    /// @}

    /// Universal constructor using Serialize
    template <typename T>
    requires(!impl::IsNativeArithmetic<T> && !impl::IsNativeString<T>)
    USERVER_IMPL_NODEBUG_INLINE_FUNC ValueBuilder(const T& t)
        : value_(GetSerializedRoot(DoSerialize(t)))
    {}

    /// @name Native value constructors
    /// @{
    /// @brief Constructs a JSON null.
    ValueBuilder(std::nullptr_t)
        : ValueBuilder()
    {}

    /// @brief Access member by key for modification.
    /// @throw `TypeMismatchException` if not object or null value.
    ValueBuilder operator[](std::string key);
    /// @brief Access array member by index for modification.
    /// @throw `TypeMismatchException` if not an array value.
    /// @throw `OutOfBoundsException` if index is greater than size.
    ValueBuilder operator[](std::size_t index);
    /// @brief Access member by key for modification.
    /// @throw `TypeMismatchException` if not object or null value.
    template <typename Tag, utils::StrongTypedefOps Ops>
    requires(utils::IsStrongTypedefLoggable(Ops))
    ValueBuilder operator[](utils::StrongTypedef<Tag, std::string, Ops> key) {
        return (*this)[std::move(key.GetUnderlying())];
    }

    ValueBuilder& operator=(const ValueBuilder& other);
    // NOLINTNEXTLINE(cppcoreguidelines-noexcept-move-operations, performance-noexcept-move-constructor)
    ValueBuilder& operator=(ValueBuilder&& other);

    /// @brief Assigns a numeric or boolean value directly, without creating a temporary builder.
    template <typename T>
    requires(impl::IsNativeArithmetic<T> && !meta::IsCharacter<std::remove_cv_t<T>>)
    ValueBuilder& operator=(T value) {
        AssignArithmetic(NormalizeArithmetic(value));
        return *this;
    }

    /// @brief Assigns a number representing the character's code unit.
    template <typename T>
    requires(impl::IsNativeArithmetic<T> && meta::IsCharacter<std::remove_cv_t<T>>)
    [[deprecated(
        "Characters will be converted into underlying integer type. Avoid this or wrap with static_cast<int>(...) "
        "explicitly."
    )]]
    ValueBuilder& operator=(T value) {
        AssignArithmetic(NormalizeArithmetic(value));
        return *this;
    }

    /// @brief Assigns a string, copying its bytes into owned storage.
    /// The source remains unchanged and may be modified or destroyed afterwards.
    template <typename T>
    requires impl::IsNativeString<T>
    ValueBuilder& operator=(const T& value) {
        AssignString(std::string_view{value});
        return *this;
    }

    /// @brief Assigns an existing JSON value without constructing a temporary builder.
    /// Lvalues and const or shared rvalues are copied. Uniquely owned non-const rvalues are moved and become null.
    /// Copied sources and their aliases remain unchanged.
    /// @throw formats::json::MemberMissingException if the source value is missing.
    template <typename T>
    requires std::is_same_v<std::remove_cvref_t<T>, formats::json::Value>
    ValueBuilder& operator=(T&& value) {
        AssignValue(std::forward<T>(value));
        return *this;
    }

    /// @brief Assigns JSON null directly, without creating a temporary builder.
    ValueBuilder& operator=(std::nullptr_t) {
        AssignNull();
        return *this;
    }

    /// @brief Emplaces new member w/o a check whether the key already exists.
    /// @warning May create invalid JSON with duplicate key.
    /// @throw `TypeMismatchException` if not object or null value.
    void EmplaceNocheck(std::string_view key, ValueBuilder value);

    /// @brief Copies an existing JSON value directly, without constructing a temporary ValueBuilder.
    /// @overload
    void EmplaceNocheck(std::string_view key, const formats::json::Value& value);

    /// @brief Moves a uniquely owned JSON value directly, copying shared values.
    /// @overload
    void EmplaceNocheck(std::string_view key, formats::json::Value&& value);

    /// @overload
    void EmplaceNocheck(std::string_view key, std::nullptr_t);

    /// @overload
    void EmplaceNocheck(std::string_view key, const std::string& value);

    /// @overload
    void EmplaceNocheck(std::string_view key, std::string_view value);

    /// @overload
    template <typename T>
    requires std::is_convertible_v<const T&, std::string_view>
    void EmplaceNocheck(std::string_view key, const T& value) {
        if constexpr (impl::IsNativeString<T>) {
            EmplaceNocheck(key, std::string_view{value});
        } else {
            EmplaceNocheck(key, ValueBuilder{value});
        }
    }

    /// @overload
    template <typename T>
    requires(impl::IsNativeArithmetic<T> && !meta::IsCharacter<std::remove_cv_t<T>>)
    void EmplaceNocheck(std::string_view key, T value) {
        EmplaceNocheckArithmetic(key, NormalizeArithmetic(value));
    }

    /// @brief Emplaces a number representing the character's code unit.
    /// @overload
    template <typename T>
    requires(impl::IsNativeArithmetic<T> && meta::IsCharacter<std::remove_cv_t<T>>)
    [[deprecated(
        "Characters will be converted into underlying integer type. Avoid this or wrap with static_cast<int>(...) "
        "explicitly."
    )]]
    void EmplaceNocheck(std::string_view key, T value) {
        EmplaceNocheckArithmetic(key, NormalizeArithmetic(value));
    }

    /// @overload
    template <typename T>
    [[deprecated(
        "Use natively passed argument instead of curly-braces wrapped. Such usage could affect the performance."
    )]]
    void EmplaceNocheck(std::string_view key, std::initializer_list<T> initializer) {
        CheckSingleElementInitializerList(initializer.size());
        EmplaceNocheck(key, *initializer.begin());
    }

    /// @brief Remove key from object. If key is missing nothing happens.
    /// @throw `TypeMismatchException` if value is not an object.
    void Remove(std::string_view key);

    iterator begin();
    iterator end();

    /// @brief Returns whether the array or object is empty.
    /// @throw `TypeMismatchException` if not an array or an object.
    bool IsEmpty() const;

    /// @brief Returns true if *this holds a Null (Type::kNull).
    bool IsNull() const noexcept;

    /// @brief Returns true if *this is convertible to bool.
    bool IsBool() const noexcept;

    /// @brief Returns true if *this is convertible to int.
    bool IsInt() const noexcept;

    /// @brief Returns true if *this is convertible to int64_t.
    bool IsInt64() const noexcept;

    /// @brief Returns true if *this is convertible to uint64_t.
    bool IsUInt64() const noexcept;

    /// @brief Returns true if *this is convertible to double.
    bool IsDouble() const noexcept;

    /// @brief Returns true if *this is convertible to std::string.
    bool IsString() const noexcept;

    /// @brief Returns true if *this is an array (Type::kArray).
    bool IsArray() const noexcept;

    /// @brief Returns true if *this holds a map (Type::kObject).
    bool IsObject() const noexcept;

    /// @brief Returns array size or object members count.
    /// @throw `TypeMismatchException` if not an array or an object.
    std::size_t GetSize() const;

    /// @brief Returns storage capacity for array elements or object members.
    /// @throw `TypeMismatchException` if not an array or an object.
    std::size_t GetCapacity() const;

    /// @brief Returns true if value holds a `key`.
    /// @throw `TypeMismatchException` if `*this` is not a map or null.
    bool HasMember(std::string_view key) const;

    /// @brief Returns full path to this value.
    std::string GetPath() const;

    /// @brief Ensures storage capacity for array elements or object members is at least `capacity` without changing
    /// size.
    /// @throw `TypeMismatchException` if not an array or object.
    void Reserve(std::size_t capacity);

    /// @brief Resize the array value or convert null value
    /// into an array of requested size.
    /// @throw `TypeMismatchException` if not an array or null.
    void Resize(std::size_t size);

    /// @brief Appends a JSON `null` directly, without constructing a temporary `ValueBuilder`, converting this builder
    /// from null to an array if necessary.
    /// @throw `TypeMismatchException` if the builder contains neither an array nor null.
    void PushBack(std::nullptr_t);

    /// @brief Appends a string directly, without constructing a temporary `ValueBuilder`.
    /// @throw `TypeMismatchException` if the builder contains neither an array nor null.
    void PushBack(const std::string& value);

    /// @overload
    void PushBack(std::string_view value);

    /// @brief Appends a string-like value directly, without constructing a temporary `ValueBuilder`.
    /// @throw `TypeMismatchException` if the builder contains neither an array nor null.
    template <typename T>
    requires std::is_convertible_v<const T&, std::string_view>
    void PushBack(const T& value) {
        if constexpr (impl::IsNativeString<T>) {
            PushBack(std::string_view{value});
        } else {
            PushBack(ValueBuilder{value});
        }
    }

    /// @brief Appends an arithmetic value directly, without constructing a temporary `ValueBuilder`.
    /// @throw `TypeMismatchException` if the builder contains neither an array nor null.
    template <typename T>
    requires(std::is_arithmetic_v<T> && !meta::IsCharacter<std::remove_cv_t<T>>)
    void PushBack(T value) {
        if constexpr (std::is_same_v<T, long double>) {
            PushBackArithmetic(static_cast<double>(value));
        } else {
            PushBackArithmetic(NormalizeArithmetic(value));
        }
    }

    /// @brief Appends a number representing the character's code unit.
    /// @throw `TypeMismatchException` if the builder contains neither an array nor null.
    template <typename T>
    requires(std::is_arithmetic_v<T> && meta::IsCharacter<std::remove_cv_t<T>>)
    [[deprecated(
        "Characters will be converted into underlying integer type. Avoid this or wrap with static_cast<int>(...) "
        "explicitly."
    )]]
    void PushBack(T value) {
        PushBackArithmetic(NormalizeArithmetic(value));
    }

    /// @brief Appends the single element of a braced initializer.
    /// The element is forwarded to the matching `PushBack` overload, avoiding a temporary `ValueBuilder` when a direct
    /// overload is available.
    /// @throw `Exception` if the initializer does not contain exactly one element.
    /// @throw `TypeMismatchException` if the builder contains neither an array nor null.
    template <typename T>
    [[deprecated(
        "Use natively passed argument instead of curly-braces wrapped. Such usage could affect the performance."
    )]]
    void PushBack(std::initializer_list<T> initializer) {
        CheckSingleElementInitializerList(initializer.size());
        if constexpr (std::is_same_v<T, ValueBuilder>) {
            PushBack(ValueBuilder{*initializer.begin()});
        } else {
            PushBack(*initializer.begin());
        }
    }

    /// @brief Add element into the last position of array.
    /// @throw `TypeMismatchException` if not an array or null.
    void PushBack(ValueBuilder&& bld);

    /// @brief Copy an existing JSON value into the last position of array.
    /// @throw `TypeMismatchException` if not an array or null.
    void PushBack(const formats::json::Value& value);

    /// @brief Move an existing JSON value into the last position of array when
    /// it is uniquely owned, otherwise copy it.
    /// @throw `TypeMismatchException` if not an array or null.
    void PushBack(formats::json::Value&& value);

    /// @brief Take out the resulting `Value` object.
    /// After calling this method the object is in unspecified
    /// (but valid - possibly null) state.
    /// @throw `json::Exception` if called not from the root builder.
    formats::json::Value ExtractValue();

private:
    class EmplaceEnabler {};

public:
    /// @cond
    ValueBuilder(EmplaceEnabler, impl::MutableValueWrapper) noexcept;
    /// @endcond

private:
    enum class CheckMemberExists { kYes, kNo };

    explicit ValueBuilder(impl::MutableValueWrapper) noexcept;

    static void Copy(impl::Value& to, const ValueBuilder& from);
    static void Move(impl::Value& to, ValueBuilder&& from);

    static impl::VersionedValuePtr GetSerializedRoot(Value value);
    static impl::VersionedValuePtr GetMovedValueRoot(Value& value);
    static impl::MutableValueWrapper GetMovedWrapper(ValueBuilder& other);

    static impl::VersionedValuePtr CreateString(std::string_view value);

    template <typename T>
    requires impl::IsNormalizedArithmetic<T>
    static impl::VersionedValuePtr CreateArithmetic(T value);

    void AssignNative(impl::Value value);
    void AssignNull();
    void AssignString(std::string_view value);
    void AssignValue(const Value& value);
    void AssignValue(Value&& value);
    template <typename T>
    requires impl::IsNormalizedArithmetic<T>
    void AssignArithmetic(T value);

    template <typename T>
    requires std::is_arithmetic_v<T>
    static constexpr auto NormalizeArithmetic(T value) noexcept {
        if constexpr (std::is_same_v<T, bool> || std::is_same_v<T, int> || std::is_same_v<T, unsigned int> ||
                      std::is_same_v<T, float> || std::is_same_v<T, double>)
        {
            return value;
        } else {
            static_assert(
                std::is_integral_v<T> && sizeof(T) <= sizeof(std::int64_t),
                "JSON arithmetic normalization requires an integer no wider than 64 bits"
            );
            if constexpr (std::is_signed_v<T>) {
                return static_cast<std::int64_t>(value);
            } else {
                return static_cast<std::uint64_t>(value);
            }
        }
    }

    template <typename T>
    requires impl::IsNormalizedArithmetic<T>
    void EmplaceNocheckArithmetic(std::string_view key, T value);

    template <typename T>
    requires impl::IsNormalizedArithmetic<T>
    void PushBackArithmetic(T value);

    template <typename MakeValue>
    USERVER_IMPL_NODEBUG_INLINE_FUNC inline void PushBackNative(MakeValue&& make_value);
    static void CheckSingleElementInitializerList(std::size_t size);

    impl::Value& AddMember(std::string_view key, CheckMemberExists);

    template <typename MakeValue>
    USERVER_IMPL_NODEBUG_INLINE_FUNC inline impl::Value& AddMember(
        std::string_view key,
        CheckMemberExists,
        MakeValue&& make_value
    );

    template <typename T>
    USERVER_IMPL_NODEBUG_INLINE_FUNC static Value DoSerialize(const T& t) {
        static_assert(
            formats::common::impl::HasSerialize<Value, T>,
            "There is no `Serialize(const T&, formats::serialize::To<json::Value>)` "
            "in namespace of `T` or `formats::serialize`. "
            ""
            "Probably you forgot to include the <userver/formats/serialize/common_containers.hpp> header "
            "or one of the <formats/json/serialize_*.hpp> headers or you have not provided a `Serialize` function "
            "overload."
        );

        return Serialize(t, formats::serialize::To<Value>());
    }

    impl::MutableValueWrapper value_;

    friend class Iterator<IterTraits, common::IteratorDirection::kForward>;
    friend class Iterator<IterTraits, common::IteratorDirection::kReverse>;
};

template <typename T>
requires(std::is_integral_v<T> && sizeof(T) <= sizeof(std::int64_t))
Value Serialize(T value, formats::serialize::To<Value>) {
    using Type = std::conditional_t<std::is_signed<T>::value, int64_t, uint64_t>;
    return json::ValueBuilder(static_cast<Type>(value)).ExtractValue();
}

json::Value Serialize(std::chrono::system_clock::time_point tp, formats::serialize::To<Value>);

/// Optimized maps of StrongTypedefs serialization for JSON
template <typename T>
requires(meta::IsUniqueMap<T> && utils::IsStrongTypedefLoggable(T::key_type::kOps))
Value Serialize(const T& value, formats::serialize::To<Value>) {
    json::ValueBuilder builder(formats::common::Type::kObject);
    for (const auto& [key, value] : value) {
        builder.EmplaceNocheck(key.GetUnderlying(), value);
    }
    return builder.ExtractValue();
}

/// Optimized maps serialization for JSON
template <typename T>
requires meta::IsUniqueMap<T> && std::is_convertible_v<typename T::key_type, std::string>
Value Serialize(const T& value, formats::serialize::To<Value>) {
    json::ValueBuilder builder(formats::common::Type::kObject);
    for (const auto& [key, value] : value) {
        builder.EmplaceNocheck(key, value);
    }
    return builder.ExtractValue();
}

}  // namespace formats::json

USERVER_NAMESPACE_END
