#pragma once

/// @file userver/storages/postgres/io/enum_types.hpp
/// @brief Enum I/O support
/// @ingroup userver_postgres_parse_and_format

#include <string_view>
#include <unordered_map>

#include <userver/storages/postgres/io/buffer_io.hpp>
#include <userver/storages/postgres/io/buffer_io_base.hpp>
#include <userver/storages/postgres/io/pg_types.hpp>
#include <userver/storages/postgres/io/type_mapping.hpp>
#include <userver/storages/postgres/io/user_types.hpp>
#include <userver/utils/trivial_map_fwd.hpp>

#include <userver/storages/postgres/detail/string_hash.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::io {

// clang-format off

/// @page pg_enum uPg: Mapping a C++ enum to PostgreSQL enum type.
///
/// A C++ enumeration can be mapped to a PostgreSQL enum type by providing a
/// list of PostgreSQL literals mapped to the enumeration values.
///
/// For example, if we have a C++ enumeration declared as follows:
/// @snippet storages/postgres/tests/enums_pgtest.cpp C++ enum type
/// and a PostgreSQL enum type:
/// @code
/// create type __pgtest.rainbow as enum (
///  'red', 'orange', 'yellow', 'green', 'cyan'
/// )
/// @endcode
/// all we need to do to declare the mapping between the C++ type and PosgtreSQL
/// enumeration is provide a specialization of CppToUserPg template.
/// The difference from mapping a C++ type to other user PosgreSQL types, for an
/// enumeration we need to provide a list of enumerators with corresponding
/// literals.
///
/// @warning The type mapping specialization **must** be accessible at the
/// points where parsing/formatting of the C++ type is instantiated. The
/// header where the C++ type is declared is an appropriate place to do it.
///
/// @snippet storages/postgres/tests/enums_pgtest.cpp C++ to Pg mapping
///
/// The specialization of CppToUserPg derives from EnumMappingBase for it to
/// provide type aliases for EnumeratorList and EnumType. EnumType is an alias
/// to the enumeration type for convenience of declaring pairs, as the
/// enumeration can have a long qualified name.
///
/// There is an alternative way to specialize the CppToUserPg template using
/// TrivialBiMap. This way is much more efficient, so it is preferable to use
/// it. It also becomes possible to reuse an existing TrivialBiMap.
///
/// @snippet storages/postgres/tests/enums_pgtest.cpp C++ to Pg TrivialBiMap mapping
///
/// ----------
///
/// @htmlonly <div class="bottom-nav"> @endhtmlonly
/// ⇦ @ref pg_composite_types | @ref pg_range_types ⇨
/// @htmlonly </div> @endhtmlonly

// clang-format on

namespace detail {

/// Enumerator literal value holder
template <typename Enum>
struct Enumerator {
  Enum enumerator;
  std::string_view literal;

  constexpr Enumerator(Enum en, std::string_view lit)
      : enumerator{en}, literal{lit} {}
};

}  // namespace detail

/// Base template for providing type aliases for defining enumeration
/// mapping.
template <typename Enum>
struct EnumMappingBase {
  static_assert(std::is_enum<Enum>(), "Type must be an enumeration");
  /// @brief Type alias for the enumeration.
  ///
  /// As the mapping must be specialized in `storages::postgres::io` namespace,
  /// the enumerator value can be quite a long name. This type alias is a
  /// shortcut to the enumeration type.
  using EnumType = Enum;
  /// Type alias for enumerator literal value holder
  using Enumerator = detail::Enumerator<Enum>;
  /// @brief Type alias for enumerator-literal mapping.
  ///
  /// See @ref pg_enum
  using EnumeratorList = const std::initializer_list<Enumerator>;
};

namespace detail {

template <typename Enum, typename Enable = USERVER_NAMESPACE::utils::void_t<>>
struct AreEnumeratorsDefined : std::false_type {};

template <typename Enum>
struct AreEnumeratorsDefined<
    Enum,
    USERVER_NAMESPACE::utils::void_t<decltype(CppToUserPg<Enum>::enumerators)*>>
    : std::true_type {};

template <typename Enum>
struct Enumerators {
  static_assert(std::is_enum<Enum>(), "Type must be an enumeration");
  static_assert(AreEnumeratorsDefined<Enum>(),
                "CppToUserPg for an enumeration must contain a static "
                "`enumerators` member of `utils::TrivialBiMap` type or "
                "`storages::postgres::io::detail::Enumerator[]`");
  using Type = decltype(CppToUserPg<Enum>::enumerators);
};

template <typename Enum, typename = typename Enumerators<Enum>::Type>
class EnumerationMap;

template <typename Enum, typename T>
class EnumerationMap {
  using StringType = std::string_view;
  using EnumType = Enum;
  using MappingType = CppToUserPg<EnumType>;
  using EnumeratorType = Enumerator<EnumType>;

  struct EnumHash {
    std::size_t operator()(EnumType v) const {
      using underlying_type = std::underlying_type_t<EnumType>;
      using hasher = std::hash<underlying_type>;
      return hasher{}(static_cast<underlying_type>(v));
    }
  };

  using EnumToString = std::unordered_map<EnumType, StringType, EnumHash>;
  using StringToEnum =
      std::unordered_map<StringType, EnumType, utils::StringViewHash>;
  using MapsPair = std::pair<EnumToString, StringToEnum>;

  static MapsPair MapLiterals() {
    MapsPair maps;
    for (const auto& enumerator : enumerators) {
      maps.first.insert(
          std::make_pair(enumerator.enumerator, enumerator.literal));
      maps.second.insert(
          std::make_pair(enumerator.literal, enumerator.enumerator));
    }
    return maps;
  }
  static const MapsPair& GetMaps() {
    static auto maps_ = MapLiterals();
    return maps_;
  }
  static const EnumToString& EnumeratorMap() { return GetMaps().first; }
  static const StringToEnum& LiteralMap() { return GetMaps().second; }

 public:
  static constexpr const auto& enumerators = MappingType::enumerators;
  static constexpr std::size_t size = std::size(enumerators);
  static EnumType GetEnumerator(StringType literal) {
    static const auto& map = LiteralMap();
    if (auto f = map.find(literal); f != map.end()) {
      return f->second;
    }
    throw InvalidEnumerationLiteral{
        compiler::GetTypeName<EnumType>(),
        std::string{literal.data(), literal.size()}};
  }
  static StringType GetLiteral(EnumType enumerator) {
    static const auto& map = EnumeratorMap();
    if (auto f = map.find(enumerator); f != map.end()) {
      return f->second;
    }
    throw InvalidEnumerationValue{enumerator};
  }
};

template <typename Enum, typename Func>
class EnumerationMap<Enum, const USERVER_NAMESPACE::utils::TrivialBiMap<Func>> {
  using StringType = std::string_view;
  using EnumType = Enum;
  using MappingType = CppToUserPg<EnumType>;

 public:
  static constexpr const auto& enumerators = MappingType::enumerators;
  static constexpr std::size_t size = enumerators.size();
  static constexpr EnumType GetEnumerator(StringType literal) {
    auto enumerator = enumerators.TryFind(literal);
    if (enumerator.has_value()) return *enumerator;
    throw InvalidEnumerationLiteral{
        compiler::GetTypeName<EnumType>(),
        std::string{literal.data(), literal.size()}};
  }
  static constexpr StringType GetLiteral(EnumType enumerator) {
    auto literal = enumerators.TryFind(enumerator);
    if (literal.has_value()) return *literal;
    throw InvalidEnumerationValue{enumerator};
  }
};

template <typename Enum>
struct EnumParser : BufferParserBase<Enum> {
  using BaseType = BufferParserBase<Enum>;
  using EnumMap = EnumerationMap<Enum>;

  using BaseType::BaseType;

  void operator()(const FieldBuffer& buffer) {
    std::string_view literal;
    io::ReadBuffer(buffer, literal);
    this->value = EnumMap::GetEnumerator(literal);
  }
};

template <typename Enum>
struct EnumFormatter : BufferFormatterBase<Enum> {
  using BaseType = BufferFormatterBase<Enum>;
  using EnumMap = EnumerationMap<Enum>;

  using BaseType::BaseType;

  template <typename Buffer>
  void operator()(const UserTypes& types, Buffer& buffer) const {
    auto literal = EnumMap::GetLiteral(this->value);
    io::WriteBuffer(types, buffer, literal);
  }
};

}  // namespace detail

namespace traits {

template <typename T>
struct Input<T,
             std::enable_if_t<!detail::kCustomParserDefined<T> &&
                              std::is_enum<T>() && IsMappedToUserType<T>()>> {
  using type = io::detail::EnumParser<T>;
};

template <typename T>
struct ParserBufferCategory<io::detail::EnumParser<T>>
    : std::integral_constant<BufferCategory, BufferCategory::kPlainBuffer> {};

template <typename T>
struct Output<T,
              std::enable_if_t<!detail::kCustomFormatterDefined<T> &&
                               std::is_enum<T>() && IsMappedToUserType<T>()>> {
  using type = io::detail::EnumFormatter<T>;
};

}  // namespace traits

}  // namespace storages::postgres::io

USERVER_NAMESPACE_END
