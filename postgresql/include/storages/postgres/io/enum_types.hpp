#pragma once

/// @file storages/postgres/io/enum_types.hpp
/// @brief Enum I/O support

#include <unordered_map>

#include <storages/postgres/io/buffer_io.hpp>
#include <storages/postgres/io/buffer_io_base.hpp>
#include <storages/postgres/io/pg_types.hpp>
#include <storages/postgres/io/type_mapping.hpp>
#include <storages/postgres/io/user_types.hpp>

#include <storages/postgres/detail/string_hash.hpp>

#include <utils/string_view.hpp>
#include <utils/strlen.hpp>

namespace storages::postgres::io {

/// @page pg_enum µPg: Mapping a C++ enum to PostgreSQL enum type.
///
/// A C++ enumeration can be mapped to a PostgreSQL enum type by providing a
/// list of PostgreSQL literals mapped to the enumeration values.
///
/// For example, if we have a C++ enumeration declared as follows:
/// @snippet storages/postgres/tests/enums_pgtest.cpp C++ enum type
/// and a PostgreSQL enum type:
/// @code
/// create type __pgtest.rainbow as enum (
///  'red', 'orange', 'yellow', 'green', 'cyan', 'blue', 'violet'
/// )
/// @endcode
/// all we need to do to declare the mapping between the C++ type and PosgtreSQL
/// enumeration is provide a specialisation of CppToUserPg template.
/// The difference from mapping a C++ type to other user PosgreSQL types, for an
/// enumeration we need to provide a list of enumerators with corresponding
/// literals.
///
/// @warning The type mapping specialisation **must** be accessible at the
/// points where parsing/formatting of the C++ type is instantiated. The
/// header where the C++ type is declared is an appropriate place to do it.
///
/// @snippet storages/postgres/tests/enums_pgtest.cpp C++ to Pg mapping
///
/// The specialisation of CppToUserPg derives from EnumMappingBase for it to
/// provide type aliases for EnumeratorList and EnumType. EnumType is an alias
/// to the enumeration type for convenience of declaring pairs, as the
/// enumeration can have a long qualified name.
///

namespace detail {

/// Enumerator literal value holder
template <typename Enum>
struct Enumerator {
  Enum enumerator;
  ::utils::string_view literal;

  constexpr Enumerator(Enum en, const char* lit)
      : enumerator{en}, literal{lit, ::utils::StrLen(lit)} {}
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

template <typename Enum, typename Enable = ::utils::void_t<>>
struct AreEnumeratorsDefined : std::false_type {};

template <typename Enum>
struct AreEnumeratorsDefined<
    Enum, ::utils::void_t<decltype(CppToUserPg<Enum>::enumerators)*>>
    : std::true_type {};

template <typename Enum>
class EnumerationMap {
  static_assert(std::is_enum<Enum>(), "Type must be an enumeration");
  static_assert(AreEnumeratorsDefined<Enum>(),
                "CppToUserPg for an enumeration must contain a static "
                "`enumerators` member");

  using StringType = ::utils::string_view;
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

 private:
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
  static constexpr auto enumerators = MappingType::enumerators;
  static constexpr std::size_t size = enumerators.size();
  static EnumType GetEnumerator(StringType literal) {
    static const auto& map = LiteralMap();
    if (auto f = map.find(literal); f != map.end()) {
      return f->second;
    }
    throw InvalidEnumerationLiteral{
        ::compiler::GetTypeName<EnumType>(),
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

template <typename Enum, DataFormat F>
struct EnumParser : BufferParserBase<Enum> {
  using BaseType = BufferParserBase<Enum>;
  using EnumMap = EnumerationMap<Enum>;

  using BaseType::BaseType;

  void operator()(const FieldBuffer& buffer) {
    ::utils::string_view literal;
    ReadBuffer<F>(buffer, literal);
    this->value = EnumMap::GetEnumerator(literal);
  }
};

template <typename Enum, DataFormat F>
struct EnumFormatter : BufferFormatterBase<Enum> {
  using BaseType = BufferFormatterBase<Enum>;
  using EnumMap = EnumerationMap<Enum>;

  using BaseType::BaseType;

  template <typename Buffer>
  void operator()(const UserTypes& types, Buffer& buffer) const {
    auto literal = EnumMap::GetLiteral(this->value);
    WriteBuffer<F>(types, buffer, literal);
  }
};

}  // namespace detail

namespace traits {

template <typename T, DataFormat F>
struct Input<T, F,
             std::enable_if_t<!detail::kCustomParserDefined<T, F> &&
                              std::is_enum<T>() && IsMappedToUserType<T>()>> {
  using type = io::detail::EnumParser<T, F>;
};

template <typename T, DataFormat F>
struct ParserBufferCategory<io::detail::EnumParser<T, F>>
    : std::integral_constant<BufferCategory, BufferCategory::kPlainBuffer> {};

template <typename T, DataFormat F>
struct Output<T, F,
              std::enable_if_t<!detail::kCustomFormatterDefined<T, F> &&
                               std::is_enum<T>() && IsMappedToUserType<T>()>> {
  using type = io::detail::EnumFormatter<T, F>;
};

}  // namespace traits

}  // namespace storages::postgres::io
