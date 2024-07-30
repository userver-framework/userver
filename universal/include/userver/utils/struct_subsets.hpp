#pragma once

/// @file userver/utils/struct_subsets.hpp
/// @brief Utilities for creating a struct with a subset of fields of the
/// original struct, with conversions between them.

#include <type_traits>
#include <utility>

#include <boost/preprocessor/empty.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

#include <userver/utils/impl/boost_variadic_to_seq.hpp>
#include <userver/utils/impl/internal_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

struct RequireSemicolon;

struct NonMovable final {
  constexpr explicit NonMovable(InternalTag) noexcept {}
};

template <typename T>
constexpr auto IsDefinedAndAggregate()
    -> decltype(static_cast<void>(sizeof(T)), false) {
  return std::is_aggregate_v<T>;
}

template <typename /*T*/, typename... Args>
constexpr auto IsDefinedAndAggregate(Args...) -> bool {
  return false;
}

}  // namespace utils::impl

USERVER_NAMESPACE_END

/// @cond

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_IMPL_STRUCT_MAP(r, data, elem) \
  std::forward<OtherDeps>(other).elem,

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_IMPL_MAKE_FROM_SUPERSET(Self, ...)                             \
  template <typename OtherDeps>                                                \
  static Self MakeFromSupersetImpl(                                            \
      OtherDeps&& other, USERVER_NAMESPACE::utils::impl::InternalTag) {        \
    return {BOOST_PP_SEQ_FOR_EACH(USERVER_IMPL_STRUCT_MAP, BOOST_PP_EMPTY(),   \
                                  USERVER_IMPL_VARIADIC_TO_SEQ(__VA_ARGS__))}; \
  }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_IMPL_STRUCT_SUBSET_MAP(r, data, elem) \
  /* NOLINTNEXTLINE(bugprone-macro-parentheses) */    \
  decltype(data::elem) elem;

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_IMPL_STRUCT_SUBSET_REF_MAP(r, data, elem) \
  /* NOLINTNEXTLINE(bugprone-macro-parentheses) */        \
  std::add_const_t<decltype(data::elem)>& elem;

/// @endcond

/// @brief Should be invoked inside a manually defined "root" struct to enable
/// conversions from it to subset structs created by
/// @ref USERVER_MAKE_STRUCT_SUBSET and @ref USERVER_MAKE_STRUCT_SUBSET_REF.
///
/// @hideinitializer
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_ALLOW_CONVERSIONS_TO_SUBSET()                                  \
  template <                                                                   \
      typename Other,                                                          \
      std::enable_if_t<                                                        \
          USERVER_NAMESPACE::utils::impl::IsDefinedAndAggregate<Other>(), int> \
          Enable = 0>                                                          \
  /*implicit*/ operator Other() const& {                                       \
    return Other::MakeFromSupersetImpl(                                        \
        *this, USERVER_NAMESPACE::utils::impl::InternalTag{});                 \
  }                                                                            \
                                                                               \
  template <                                                                   \
      typename Other,                                                          \
      std::enable_if_t<                                                        \
          USERVER_NAMESPACE::utils::impl::IsDefinedAndAggregate<Other>(), int> \
          Enable = 0>                                                          \
  /*implicit*/ operator Other()&& {                                            \
    return Other::MakeFromSupersetImpl(                                        \
        std::move(*this), USERVER_NAMESPACE::utils::impl::InternalTag{});      \
  }                                                                            \
                                                                               \
  friend struct USERVER_NAMESPACE::utils::impl::RequireSemicolon

/// @brief Defines a struct containing a subset of data members
/// from @a OriginalDependencies.
///
/// Implicit conversions (by copy and by move) are allowed from any superset
/// struct to the @a SubsetStruct, as long as the names of the data members
/// match, and the superset struct is either defined using
/// @ref USERVER_MAKE_STRUCT_SUBSET or @ref USERVER_MAKE_STRUCT_SUBSET_REF,
/// or it contains @ref USERVER_ALLOW_CONVERSIONS_TO_SUBSET.
///
/// Usage example:
/// @snippet utils/struct_subsets_test.cpp  deps definitions
/// @snippet utils/struct_subsets_test.cpp  deps usage
///
/// @param SubsetStruct the name of the subset struct to define
/// @param OriginalStruct the name of the superset struct, including its
/// namespace if needed
/// @param ... names of the data members to copy
///
/// @hideinitializer
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_DEFINE_STRUCT_SUBSET(SubsetStruct, OriginalStruct, ...)   \
  struct SubsetStruct {                                                   \
    BOOST_PP_SEQ_FOR_EACH(USERVER_IMPL_STRUCT_SUBSET_MAP, OriginalStruct, \
                          USERVER_IMPL_VARIADIC_TO_SEQ(__VA_ARGS__))      \
                                                                          \
    USERVER_IMPL_MAKE_FROM_SUPERSET(SubsetStruct, __VA_ARGS__)            \
                                                                          \
    USERVER_ALLOW_CONVERSIONS_TO_SUBSET();                                \
  }

/// @brief Defines a struct containing a subset of data members
/// from @a OriginalDependencies. Appends `const&` to types of all non-reference
/// data members.
///
/// Implicit conversions (by copy and by move) are allowed from any superset
/// struct to the @a SubsetStruct, as long as the names of the data members
/// match, and the superset struct is either defined using
/// @ref USERVER_MAKE_STRUCT_SUBSET or @ref USERVER_MAKE_STRUCT_SUBSET_REF,
/// or it contains @ref USERVER_ALLOW_CONVERSIONS_TO_SUBSET.
///
/// `*Ref` structs can be used for parameters of utility functions to avoid
/// copying non-reference data members.
///
/// Usage example:
/// @snippet utils/struct_subsets_test.cpp  ref definitions
/// @snippet utils/struct_subsets_test.cpp  ref usage
///
/// @param SubsetStructRef the name of the subset struct to define, it should
/// typically contain `*Ref` suffix to underline that it needs the original
/// backing struct to work
/// @param OriginalStruct the name of the superset struct, including its
/// namespace if needed
/// @param ... names of the data members to copy
///
/// @hideinitializer
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_DEFINE_STRUCT_SUBSET_REF(SubsetStructRef, OriginalStruct, ...) \
  struct SubsetStructRef {                                                     \
    BOOST_PP_SEQ_FOR_EACH(USERVER_IMPL_STRUCT_SUBSET_REF_MAP, OriginalStruct,  \
                          USERVER_IMPL_VARIADIC_TO_SEQ(__VA_ARGS__))           \
                                                                               \
    /* Protects against copying into async functions */                        \
    USERVER_NAMESPACE::utils::impl::NonMovable _impl_non_movable{              \
        USERVER_NAMESPACE::utils::impl::InternalTag{}};                        \
                                                                               \
    USERVER_IMPL_MAKE_FROM_SUPERSET(SubsetStructRef, __VA_ARGS__)              \
                                                                               \
    USERVER_ALLOW_CONVERSIONS_TO_SUBSET();                                     \
  }
