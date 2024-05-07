#pragma once

#include <string_view>
#include <type_traits>

namespace NYdb {
class TType;
class TValue;
class TValueParser;
class TValueBuilder;
class TParamValueBuilder;

template <typename Builder>
class TValueBuilderBase;
}  // namespace NYdb

USERVER_NAMESPACE_BEGIN

namespace ydb {

/// Used for error reporting in ydb::ValueTraits::Parse.
struct ParseContext final {
  std::string_view column_name;
};

template <typename T, typename Enable = void>
struct ValueTraits {
  /// Parses an element of type `T` from `parser`. `context` may be used
  /// for diagnostic messages.
  static T Parse([[maybe_unused]] NYdb::TValueParser& parser,
                 [[maybe_unused]] const ParseContext& context) {
    static_assert(!sizeof(T),
                  "Cannot convert from YDB value. No ValueTraits defined for "
                  "this type. userver/ydb/io/supported_types.hpp "
                  "contains all known definitions");
    return T{};
  }

  /// Writes an element of type `T` to `builder`.
  template <typename Builder>
  static void Write([[maybe_unused]] NYdb::TValueBuilderBase<Builder>& builder,
                    [[maybe_unused]] const T& value) {
    static_assert(!sizeof(T),
                  "Cannot convert to YDB value. No ValueTraits defined for "
                  "this type. userver/ydb/io/supported_types.hpp "
                  "contains all known definitions");
  }

  /// Uses NYdb::TTypeBuilder to create a representation of `T` type.
  static NYdb::TType MakeType() = delete;
};

/// A shorthand for calling ydb::ValueTraits<T>::Parse.
template <typename T>
inline constexpr auto Parse =
    [](NYdb::TValueParser& parser, const ParseContext& context) -> T {
  // Note: using a CPO instead of a global function to avoid ADL.
  // Customization should be performed using ValueTraits.
  return ValueTraits<T>::Parse(parser, context);
};

/// A shorthand for calling ydb::ValueTraits<T>::Write.
inline constexpr auto Write = [](auto& builder, auto&& value) {
  // Note: using a CPO instead of a global function to avoid ADL.
  // Customization should be performed using ValueTraits.
  using RawValueType =
      std::remove_const_t<std::remove_reference_t<decltype(value)>>;
  using ValueType =
      std::conditional_t<std::is_convertible_v<RawValueType, std::string_view>,
                         std::string, RawValueType>;
  ValueTraits<ValueType>::Write(builder, value);
};

}  // namespace ydb

USERVER_NAMESPACE_END
