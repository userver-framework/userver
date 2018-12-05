#pragma once

#include <unordered_map>

#include <storages/postgres/exceptions.hpp>

#include <storages/postgres/io/pg_types.hpp>
#include <storages/postgres/io/traits.hpp>
#include <storages/postgres/io/type_mapping.hpp>
#include <storages/postgres/io/user_types.hpp>

#include <storages/postgres/detail/string_hash.hpp>

#include <utils/string_view.hpp>
#include <utils/strlen.hpp>

namespace storages::postgres::io {

/// @page pg_enum Mapping a C++ enum to PostgreSQL enum type.
///
/// A C++ enumeration can be mapped to a PostgreSQL enum type by providing a
/// list of PostgreSQL literals mapped to the enumeration values.
///
/// For example, if we have a C++ enumeration declared as follows:
/// @code
/// enum class TrafficLigths {
///   kRed, kYellow, kGreen
/// };
/// @endcode
/// and a PostgreSQL enum type:
/// @code
/// create type my_schema.traffic_lights as enum (
///   'red', 'yellow', 'green'
/// );
/// @endcode
/// all we need to do to declare the mapping between the C++ type and PosgtreSQL
/// enumeration is provide a specialisation of CppToUserPg template.
/// The difference from mapping a C++ type to other user PosgreSQL types, for an
/// enumeration we need to provide a list of enumerators with corresponding
/// literals.
///
/// @code
/// namespace storages::postgres::io {
/// template <>
/// struct CppToUserPg<TrafficLights> : EnumMappingBase<TrafficLights> {
///   static constexpr DBTypeName postgres_name = "my_schema.traffic_lights";
///   static constexpr EnumeratorList enumerators {
///     {EnumType::kRed, "red"},
///     {EnumType::kYellow, "yellow"},
///     {EnumType::kGreen, "green"}
///   };
/// };
/// } // namespace
/// @endcode
///
/// The specialisation of CppToUserPg derives from EnumMappingBase for it to
/// provide type aliases for EnumeratorList and EnumType. EnumType is an alias
/// to the enumeration type for convenience of declaring pairs, as the
/// enumeration can have a long qualified name.
///

namespace detail {

template <typename Enum>
struct Enumerator {
  Enum enumerator;
  ::utils::string_view literal;

  constexpr Enumerator(Enum en, const char* lit)
      : enumerator{en}, literal{lit, ::utils::StrLen(lit)} {}
};

}  // namespace detail

template <typename Enum>
struct EnumMappingBase {
  static_assert(std::is_enum<Enum>(), "Type must be an enumeration");
  using EnumType = Enum;
  using Enumerator = detail::Enumerator<Enum>;
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
    for (auto const& enumerator : enumerators) {
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
        ::utils::GetTypeName<EnumType>(),
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
struct Output<T, F,
              std::enable_if_t<!detail::kCustomFormatterDefined<T, F> &&
                               std::is_enum<T>() && IsMappedToUserType<T>()>> {
  using type = io::detail::EnumFormatter<T, F>;
};

}  // namespace traits

}  // namespace storages::postgres::io
