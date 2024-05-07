#pragma once

#include <optional>
#include <string>

#include <userver/formats/json_fwd.hpp>

#include <userver/ydb/io/traits.hpp>
#include <userver/ydb/types.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb {

template <typename PrimitiveTrait>
struct OptionalPrimitiveTraits {
  static std::optional<typename PrimitiveTrait::Type> Parse(
      NYdb::TValueParser& parser, const ParseContext& context);

  static void Write(NYdb::TValueBuilderBase<NYdb::TValueBuilder>& builder,
                    const std::optional<typename PrimitiveTrait::Type>& value);

  static void Write(NYdb::TValueBuilderBase<NYdb::TParamValueBuilder>& builder,
                    const std::optional<typename PrimitiveTrait::Type>& value);

  static NYdb::TType MakeType();
};

template <typename PrimitiveTrait>
struct PrimitiveTraits {
  static typename PrimitiveTrait::Type Parse(NYdb::TValueParser& parser,
                                             const ParseContext& context);

  static void Write(NYdb::TValueBuilderBase<NYdb::TValueBuilder>& builder,
                    const typename PrimitiveTrait::Type& value);

  static void Write(NYdb::TValueBuilderBase<NYdb::TParamValueBuilder>& builder,
                    const typename PrimitiveTrait::Type& value);

  static NYdb::TType MakeType();
};

struct BoolTrait {
  using Type = bool;
  static Type Parse(const NYdb::TValueParser& value_parser);
  template <typename Builder>
  static void Write(NYdb::TValueBuilderBase<Builder>& builder, Type value);
};

struct Int32Trait {
  using Type = std::int32_t;
  static Type Parse(const NYdb::TValueParser& value_parser);
  template <typename Builder>
  static void Write(NYdb::TValueBuilderBase<Builder>& builder, Type value);
};

struct Uint32Trait {
  using Type = std::uint32_t;
  static Type Parse(const NYdb::TValueParser& value_parser);
  template <typename Builder>
  static void Write(NYdb::TValueBuilderBase<Builder>& builder, Type value);
};

struct Int64Trait {
  using Type = std::int64_t;
  static Type Parse(const NYdb::TValueParser& value_parser);
  template <typename Builder>
  static void Write(NYdb::TValueBuilderBase<Builder>& builder, Type value);
};

struct Uint64Trait {
  using Type = std::uint64_t;
  static Type Parse(const NYdb::TValueParser& value_parser);
  template <typename Builder>
  static void Write(NYdb::TValueBuilderBase<Builder>& builder, Type value);
};

struct DoubleTrait {
  using Type = double;
  static Type Parse(const NYdb::TValueParser& value_parser);
  template <typename Builder>
  static void Write(NYdb::TValueBuilderBase<Builder>& builder, Type value);
};

struct StringTrait {
  using Type = std::string;
  static Type Parse(const NYdb::TValueParser& value_parser);
  template <typename Builder>
  static void Write(NYdb::TValueBuilderBase<Builder>& builder,
                    const Type& value);
};

struct Utf8Trait {
  using Type = Utf8;
  static Type Parse(const NYdb::TValueParser& value_parser);
  template <typename Builder>
  static void Write(NYdb::TValueBuilderBase<Builder>& builder,
                    const Type& value);
};

struct TimestampTrait {
  using Type = Timestamp;
  static Type Parse(const NYdb::TValueParser& value_parser);
  template <typename Builder>
  static void Write(NYdb::TValueBuilderBase<Builder>& builder, Type value);
};

struct JsonTrait {
  using Type = formats::json::Value;
  static Type Parse(const NYdb::TValueParser& value_parser);
  template <typename Builder>
  static void Write(NYdb::TValueBuilderBase<Builder>& builder,
                    const Type& value);
};

struct JsonDocumentTrait {
  using Type = JsonDocument;
  static Type Parse(const NYdb::TValueParser& value_parser);
  template <typename Builder>
  static void Write(NYdb::TValueBuilderBase<Builder>& builder,
                    const Type& value);
};

template <>
struct ValueTraits<std::optional<JsonDocumentTrait::Type>>
    : OptionalPrimitiveTraits<JsonDocumentTrait> {};

template <>
struct ValueTraits<JsonDocument> : PrimitiveTraits<JsonDocumentTrait> {};

template <>
struct ValueTraits<std::optional<JsonTrait::Type>>
    : OptionalPrimitiveTraits<JsonTrait> {};

template <>
struct ValueTraits<formats::json::Value> : PrimitiveTraits<JsonTrait> {};

template <>
struct ValueTraits<std::optional<TimestampTrait::Type>>
    : OptionalPrimitiveTraits<TimestampTrait> {};

template <>
struct ValueTraits<TimestampTrait::Type> : PrimitiveTraits<TimestampTrait> {};

template <>
struct ValueTraits<std::optional<Utf8Trait::Type>>
    : OptionalPrimitiveTraits<Utf8Trait> {};

template <>
struct ValueTraits<Utf8Trait::Type> : PrimitiveTraits<Utf8Trait> {};

template <>
struct ValueTraits<std::optional<StringTrait::Type>>
    : OptionalPrimitiveTraits<StringTrait> {};

template <>
struct ValueTraits<StringTrait::Type> : PrimitiveTraits<StringTrait> {};

template <>
struct ValueTraits<std::optional<DoubleTrait::Type>>
    : OptionalPrimitiveTraits<DoubleTrait> {};

template <>
struct ValueTraits<DoubleTrait::Type> : PrimitiveTraits<DoubleTrait> {};

template <>
struct ValueTraits<std::optional<Uint64Trait::Type>>
    : OptionalPrimitiveTraits<Uint64Trait> {};

template <>
struct ValueTraits<Uint64Trait::Type> : PrimitiveTraits<Uint64Trait> {};

template <>
struct ValueTraits<std::optional<Int64Trait::Type>>
    : OptionalPrimitiveTraits<Int64Trait> {};

template <>
struct ValueTraits<Int64Trait::Type> : PrimitiveTraits<Int64Trait> {};

template <>
struct ValueTraits<std::optional<Uint32Trait::Type>>
    : OptionalPrimitiveTraits<Uint32Trait> {};

template <>
struct ValueTraits<Uint32Trait::Type> : PrimitiveTraits<Uint32Trait> {};

template <>
struct ValueTraits<std::optional<Int32Trait::Type>>
    : OptionalPrimitiveTraits<Int32Trait> {};

template <>
struct ValueTraits<Int32Trait::Type> : PrimitiveTraits<Int32Trait> {};

template <>
struct ValueTraits<std::optional<BoolTrait::Type>>
    : OptionalPrimitiveTraits<BoolTrait> {};

template <>
struct ValueTraits<BoolTrait::Type> : PrimitiveTraits<BoolTrait> {};

}  // namespace ydb

USERVER_NAMESPACE_END
