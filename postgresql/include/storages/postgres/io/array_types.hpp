#pragma once

#include <array>
#include <vector>

#include <storages/postgres/exceptions.hpp>
#include <storages/postgres/io/buffer_io_base.hpp>
#include <storages/postgres/io/field_buffer.hpp>
#include <storages/postgres/io/traits.hpp>
#include <storages/postgres/io/type_mapping.hpp>
#include <storages/postgres/io/type_traits.hpp>
#include <storages/postgres/io/user_types.hpp>

namespace storages::postgres::io {

/// @page pg_arrays µPg: Arrays
///
/// µPg provides support for PostgreSQL arrays when the element type is
/// supported by the driver, including user types.
///
/// Array parser will throw DimensionMismatch if the dimensions of C++
/// container do not match that of the buffer received from the server.
///
/// Array formatter will throw InvalidDimensions if containers on same
/// level of depth have different sizes.
///
/// Example of invalid dimensions from tests:
/// @snippet storages/postgres/tests/arrays_pg_test.cpp Invalid dimensions
///
/// @par Currently supported containers
/// - std::vector
/// - std::array

namespace traits {

template <typename Container>
struct HasFixedDimensions;

namespace detail {

template <typename Container>
struct HasFixedDimensionsImpl {
  using type =
      typename HasFixedDimensions<typename Container::value_type>::type;
};

}  // namespace detail

template <typename T>
struct HasFixedDimensions
    : std::conditional_t<kIsFixedSizeContainer<T>,
                         detail::HasFixedDimensionsImpl<T>,
                         BoolConstant<!kIsCompatibleContainer<T>>>::type {};

template <typename Container>
constexpr bool kHasFixedDimensions = HasFixedDimensions<Container>::value;

template <typename T>
struct FixedDimensions;

template <typename T>
struct DimensionSize : std::integral_constant<std::size_t, 0> {};

template <typename T, std::size_t N>
struct DimensionSize<std::array<T, N>>
    : std::integral_constant<std::size_t, N> {};

template <typename T>
constexpr std::size_t kDimensionSize = DimensionSize<T>::value;

namespace detail {

template <typename A, typename B>
struct JoinSequences;

template <typename T, T... U, T... V>
struct JoinSequences<std::integer_sequence<T, U...>,
                     std::integer_sequence<T, V...>> {
  using type = std::integer_sequence<T, U..., V...>;
};

template <typename T>
struct FixedDimensionsImpl {
  static_assert(kIsFixedSizeContainer<T>, "Container must have fixed size");
  using type = typename JoinSequences<
      std::integer_sequence<std::size_t, kDimensionSize<T>>,
      typename FixedDimensions<typename T::value_type>::type>::type;
};

template <typename T>
struct FixedDimensionsNonContainer {
  using type = std::integer_sequence<std::size_t>;
};

}  // namespace detail

template <typename T, T... Values>
constexpr std::array<T, sizeof...(Values)> MakeArray(
    const std::integer_sequence<T, Values...>&) {
  return {Values...};
}

template <typename T>
struct FixedDimensions
    : std::conditional_t<kIsFixedSizeContainer<T>,
                         detail::FixedDimensionsImpl<T>,
                         detail::FixedDimensionsNonContainer<T>> {};

}  // namespace traits

namespace detail {

template <typename Container>
struct ArrayBinaryParser : BufferParserBase<Container> {
  using BaseType = BufferParserBase<Container>;
  using ValueType = typename BaseType::ValueType;
  using ElementType = typename traits::ContainerFinalElement<Container>::type;
  constexpr static std::size_t dimensions = traits::kDimensionCount<Container>;
  using Dimensions = std::array<std::size_t, dimensions>;
  using DimensionIterator = typename Dimensions::iterator;
  using DimensionConstIterator = typename Dimensions::const_iterator;
  using ElementMapping = CppToPg<ElementType>;

  using BaseType::BaseType;

  void operator()(const FieldBuffer& buffer,
                  const TypeBufferCategory& categories) {
    using std::swap;
    static constexpr std::size_t int_size = sizeof(Integer);
    std::size_t offset{0};

    // read dimension count
    Integer dim_count{0};
    ReadBinary(
        buffer.GetSubBuffer(offset, int_size, BufferCategory::kPlainBuffer),
        dim_count);
    if (dim_count != static_cast<Integer>(dimensions) &&
        ForceReference(ElementMapping::init_)) {
      if (dim_count == 0) {
        ValueType empty{};
        swap(this->value, empty);
        return;
      }
      throw DimensionMismatch{};
    }
    offset += int_size;

    // read flags
    Integer flags{0};
    ReadBinary(
        buffer.GetSubBuffer(offset, int_size, BufferCategory::kPlainBuffer),
        flags);
    // TODO check flags
    offset += int_size;

    // read element oid
    Integer elem_oid{0};
    ReadBinary(
        buffer.GetSubBuffer(offset, int_size, BufferCategory::kPlainBuffer),
        elem_oid);
    // TODO check elem_oid
    auto elem_category = GetTypeBufferCategory(categories, elem_oid);
    offset += int_size;

    // read dimension data
    Dimensions on_the_wire;
    for (auto& dim : on_the_wire) {
      Integer dim_val, lbound;
      ReadBinary(
          buffer.GetSubBuffer(offset, int_size, BufferCategory::kPlainBuffer),
          dim_val);
      dim = dim_val;
      offset += int_size;
      ReadBinary(
          buffer.GetSubBuffer(offset, int_size, BufferCategory::kPlainBuffer),
          lbound);
      offset += int_size;
    }
    if (!CheckDimensions(on_the_wire)) {
      throw DimensionMismatch{};
    }
    // read elements
    ValueType tmp;
    ReadDimension(buffer.GetSubBuffer(offset), on_the_wire.begin(),
                  elem_category, categories, tmp);
    swap(this->value, tmp);
  }

 private:
  bool CheckDimensions(const Dimensions& dims) const {
    if constexpr (traits::kHasFixedDimensions<Container>) {
      return dims == traits::MakeArray(
                         typename traits::FixedDimensions<Container>::type{});
    }
    return CheckDimensions<ValueType>(dims.begin());
  }
  template <typename Element>
  bool CheckDimensions(DimensionConstIterator dim) const {
    if constexpr (traits::kIsFixedSizeContainer<Element>) {
      // check subdimensions
      if (*dim != traits::kDimensionSize<Element>) {
        return false;
      }
      if constexpr (traits::kDimensionCount<Element> == 1) {
        return true;
      } else {
        return CheckDimensions<typename Element::value_type>(dim + 1);
      }
    } else if constexpr (traits::kIsCompatibleContainer<Element>) {
      if constexpr (traits::kDimensionCount<Element> == 1) {
        return true;
      } else {
        return CheckDimensions<typename Element::value_type>(dim + 1);
      }
    }
    return true;
  }
  template <typename Element>
  std::size_t ReadDimension(const FieldBuffer& buffer,
                            DimensionConstIterator dim,
                            BufferCategory elem_category,
                            const TypeBufferCategory& categories,
                            Element& elem) {
    if constexpr (traits::kIsCompatibleContainer<Element>) {
      std::size_t offset = 0;
      if constexpr (traits::kCanResize<Element>) {
        elem.resize(*dim);
      }
      auto value = elem.begin();
      for (std::size_t i = 0; i < *dim; ++i) {
        if constexpr (1 < traits::kDimensionCount<Element>) {
          // read subdimensions
          offset += ReadDimension(buffer.GetSubBuffer(offset), dim + 1,
                                  elem_category, categories, *value++);
        } else {
          offset += ReadRawBinary(
              buffer.GetSubBuffer(offset, FieldBuffer::npos, elem_category),
              *value++, categories);
        }
      }
      return offset;
    }
  }

  std::size_t ReadDimension(const FieldBuffer& buffer,
                            DimensionConstIterator dim,
                            BufferCategory elem_category,
                            const TypeBufferCategory& categories,
                            std::vector<bool>& elem) {
    std::size_t offset = 0;
    elem.resize(*dim);
    auto value = elem.begin();
    for (std::size_t i = 0; i < *dim; ++i) {
      bool val{false};
      offset += ReadRawBinary(
          buffer.GetSubBuffer(offset, FieldBuffer::npos, elem_category), val,
          categories);
      *value++ = val;
    }
    return offset;
  }
};

template <typename Container>
struct ArrayBinaryFormatter : BufferFormatterBase<Container> {
  using BaseType = BufferFormatterBase<Container>;
  using ValueType = typename BaseType::ValueType;
  using ArrayMapping = CppToPg<Container>;
  using ElementType = typename traits::ContainerFinalElement<Container>::type;
  using ElementMapping = CppToPg<ElementType>;
  constexpr static std::size_t dimensions = traits::kDimensionCount<Container>;
  using Dimensions = std::array<std::size_t, dimensions>;
  using DimensionIterator = typename Dimensions::iterator;
  using DimensionConstIterator = typename Dimensions::const_iterator;

  using BaseType::BaseType;

  // top level container
  template <typename Buffer>
  void operator()(const UserTypes& types, Buffer& buffer,
                  Oid replace_oid = kInvalidOid) const {
    // Write number of dimensions
    WriteBinary(types, buffer, static_cast<Integer>(dimensions));
    // Write flags
    WriteBinary(types, buffer, static_cast<Integer>(0));
    // Write element type oid
    auto elem_type_oid = ElementMapping::GetOid(types);
    if (replace_oid != kInvalidOid &&
        replace_oid != ArrayMapping::GetOid(types)) {
      elem_type_oid = types.FindElementOid(replace_oid);
    }
    WriteBinary(types, buffer, static_cast<Integer>(elem_type_oid));
    Dimensions dims = GetDimensions();
    // Write data per dimension
    WriteDimensionData(types, buffer, dims);
    // Write flat elements
    WriteData(types, dims.begin(), buffer, this->value);
  }

 private:
  template <typename Element>
  void CalculateDimensions(DimensionIterator dim,
                           const Element& element) const {
    if constexpr (traits::kIsCompatibleContainer<Element>) {
      *dim = element.size();
      if (!element.empty()) {
        CalculateDimensions(dim + 1, element.front());
      }  // TODO else logic error?
    }
  }
  Dimensions GetDimensions() const {
    if constexpr (traits::kHasFixedDimensions<Container>) {
      return traits::MakeArray(
          typename traits::FixedDimensions<Container>::type{});
    } else {
      Dimensions dims;
      CalculateDimensions(dims.begin(), this->value);
      return dims;
    }
  }
  template <typename Buffer>
  void WriteDimensionData(const UserTypes& types, Buffer& buffer,
                          const Dimensions& dims) const {
    for (auto dim : dims) {
      WriteBinary(types, buffer, static_cast<Integer>(dim));
      WriteBinary(types, buffer, static_cast<Integer>(1));  // lbound
    }
  }

  template <typename Buffer, typename Element>
  void WriteData(const UserTypes& types, DimensionConstIterator dim,
                 Buffer& buffer, const Element& element) const {
    if (*dim != element.size()) {
      throw InvalidDimensions{*dim, element.size()};
    }
    if constexpr (1 < traits::kDimensionCount<Element>) {
      // this is a (sub)dimension of array
      for (const auto& sub : element) {
        WriteData(types, dim + 1, buffer, sub);
      }
    } else {
      // this is the final dimension
      for (const auto& sub : element) {
        WriteRawBinary(types, buffer, sub);
      }
    }
  }

  template <typename Buffer>
  void WriteData(const UserTypes& types, DimensionConstIterator dim,
                 Buffer& buffer, const std::vector<bool>& element) const {
    if (*dim != element.size()) {
      throw InvalidDimensions{*dim, element.size()};
    }
    for (bool sub : element) {
      WriteRawBinary(types, buffer, sub);
    }
  }
};

template <typename Container, bool System>
struct ArrayPgOid {
  using ElementType = typename traits::ContainerFinalElement<Container>::type;
  using ElementMapping = CppToPg<ElementType>;
  using Mapping = ArrayPgOid<Container, System>;

  static Oid GetOid(const UserTypes& types) {
    return ElementMapping::GetArrayOid(types);
  }
};

template <typename Container>
struct ArrayPgOid<Container, true> {
  using ElementType = typename traits::ContainerFinalElement<Container>::type;
  using ElementMapping = CppToPg<ElementType>;
  using Mapping = ArrayPgOid<Container, true>;

  static constexpr Oid GetOid(const UserTypes&) {
    return static_cast<Oid>(ElementMapping::array_oid);
  }
};

template <typename Container>
constexpr bool IsElementMappedToSystem() {
  if constexpr (!traits::kIsCompatibleContainer<Container>) {
    return false;
  } else {
    return traits::kIsMappedToSystemType<
        typename traits::ContainerFinalElement<Container>::type>;
  }
}

template <typename Container, DataFormat F>
constexpr bool EnableArrayParser() {
  if constexpr (!traits::kIsCompatibleContainer<Container>) {
    return false;
  } else {
    using ElementType = typename traits::ContainerFinalElement<Container>::type;
    return traits::kHasParser<ElementType, F>;
  }
}
template <typename Container, DataFormat F>
constexpr bool kEnableArrayParser = EnableArrayParser<Container, F>();

template <typename Container, DataFormat F>
constexpr bool EnableArrayFormatter() {
  if constexpr (!traits::kIsCompatibleContainer<Container>) {
    return false;
  } else {
    using ElementType = typename traits::ContainerFinalElement<Container>::type;
    return traits::kHasFormatter<ElementType, F>;
  }
}
template <typename Container, DataFormat F>
constexpr bool kEnableArrayFormatter = EnableArrayFormatter<Container, F>();

}  // namespace detail

template <typename T>
struct CppToPg<T, std::enable_if_t<traits::detail::EnableContainerMapping<T>()>>
    : detail::ArrayPgOid<T, detail::IsElementMappedToSystem<T>()> {};

namespace traits {

template <typename T>
struct Input<T, DataFormat::kBinaryDataFormat,
             std::enable_if_t<!detail::kCustomBinaryParserDefined<T> &&
                              io::detail::kEnableArrayParser<
                                  T, DataFormat::kBinaryDataFormat>>> {
  using type = io::detail::ArrayBinaryParser<T>;
};

template <typename T>
struct Output<T, DataFormat::kBinaryDataFormat,
              std::enable_if_t<!detail::kCustomBinaryFormatterDefined<T> &&
                               io::detail::kEnableArrayFormatter<
                                   T, DataFormat::kBinaryDataFormat>>> {
  using type = io::detail::ArrayBinaryFormatter<T>;
};

template <typename T>
struct ParserBufferCategory<io::detail::ArrayBinaryParser<T>>
    : std::integral_constant<BufferCategory, BufferCategory::kArrayBuffer> {};

// std::vector
template <typename... T>
struct IsCompatibleContainer<std::vector<T...>> : std::true_type {};

// std::array
template <typename T, std::size_t Size>
struct IsCompatibleContainer<std::array<T, Size>> : std::true_type {};
template <typename T, std::size_t Size>
struct IsFixedSizeContainer<std::array<T, Size>> : std::true_type {};

// TODO Add more containers

}  // namespace traits

}  // namespace storages::postgres::io
