#pragma once

/// @file userver/storages/postgres/io/array_types.hpp
/// @brief I/O support for arrays (std::array, std::set, std::unordered_set,
/// std::vector)
/// @ingroup userver_postgres_parse_and_format

#include <array>
#include <iterator>
#include <set>
#include <unordered_set>
#include <vector>

#include <boost/pfr/core.hpp>

#include <userver/utils/impl/projecting_view.hpp>

#include <userver/storages/postgres/exceptions.hpp>
#include <userver/storages/postgres/io/buffer_io_base.hpp>
#include <userver/storages/postgres/io/field_buffer.hpp>
#include <userver/storages/postgres/io/row_types.hpp>
#include <userver/storages/postgres/io/traits.hpp>
#include <userver/storages/postgres/io/type_mapping.hpp>
#include <userver/storages/postgres/io/type_traits.hpp>
#include <userver/storages/postgres/io/user_types.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::io {

/// @page pg_arrays uPg: Arrays
///
/// uPg provides support for PostgreSQL arrays when the element type is
/// supported by the driver, including user types.
///
/// Array parser will throw DimensionMismatch if the dimensions of C++
/// container do not match that of the buffer received from the server.
///
/// Array formatter will throw InvalidDimensions if containers on same
/// level of depth have different sizes.
///
/// Example of invalid dimensions from tests:
/// @snippet storages/postgres/tests/arrays_pgtest.cpp Invalid dimensions
///
/// @par Currently supported containers
/// - std::array
/// - std::set
/// - std::unordered_set
/// - std::vector
///
/// ----------
///
/// @htmlonly <div class="bottom-nav"> @endhtmlonly
/// ⇦ @ref pg_range_types | @ref pg_bytea ⇨
/// @htmlonly </div> @endhtmlonly

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
inline constexpr bool kHasFixedDimensions =
    HasFixedDimensions<Container>::value;

template <typename T>
struct FixedDimensions;

template <typename T>
struct DimensionSize : std::integral_constant<std::size_t, 0> {};

template <typename T, std::size_t N>
struct DimensionSize<std::array<T, N>>
    : std::integral_constant<std::size_t, N> {};

template <typename T>
inline constexpr std::size_t kDimensionSize = DimensionSize<T>::value;

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

template <typename Element>
inline bool ForceInitElementMapping() {
  // composite types can be parsed without an explicit mapping
  if constexpr (io::traits::kIsMappedToPg<Element> ||
                !io::traits::kIsCompositeType<Element>) {
    return ForceReference(CppToPg<Element>::init_);
  } else {
    return true;
  }
}

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

  void operator()(FieldBuffer buffer, const TypeBufferCategory& categories) {
    using std::swap;

    // read dimension count
    Integer dim_count{0};
    buffer.Read(dim_count, BufferCategory::kPlainBuffer);
    if (dim_count != static_cast<Integer>(dimensions) &&
        ForceInitElementMapping<ElementType>()) {
      if (dim_count == 0) {
        ValueType empty{};
        swap(this->value, empty);
        return;
      }
      throw DimensionMismatch{};
    }

    // read flags
    Integer flags{0};
    buffer.Read(flags, BufferCategory::kPlainBuffer);
    // TODO check flags

    // read element oid
    Integer elem_oid{0};
    buffer.Read(elem_oid, BufferCategory::kPlainBuffer);
    // TODO check elem_oid
    auto elem_category = GetTypeBufferCategory(categories, elem_oid);

    // read dimension data
    Dimensions on_the_wire;
    for (auto& dim : on_the_wire) {
      Integer dim_val = 0;
      buffer.Read(dim_val, BufferCategory::kPlainBuffer);
      dim = dim_val;

      Integer lbound = 0;
      buffer.Read(lbound, BufferCategory::kPlainBuffer);
    }
    if (!CheckDimensions(on_the_wire)) {
      throw DimensionMismatch{};
    }
    // read elements
    ValueType tmp;
    ReadDimension(buffer, on_the_wire.begin(), elem_category, categories, tmp);
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
  bool CheckDimensions([[maybe_unused]] DimensionConstIterator dim) const {
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

  template <typename T>
  auto GetInserter(T& value) {
    return std::inserter(value, value.end());
  }

  template <typename T, std::size_t n>
  auto GetInserter(std::array<T, n>& array) {
    return array.begin();
  }

  template <typename Element>
  void ReadDimension(FieldBuffer& buffer, DimensionConstIterator dim,
                     BufferCategory elem_category,
                     const TypeBufferCategory& categories, Element& elem) {
    if constexpr (traits::kIsCompatibleContainer<Element>) {
      if constexpr (traits::kCanClear<Element>) {
        elem.clear();
      }
      if constexpr (traits::kCanReserve<Element>) {
        elem.reserve(*dim);
      }
      auto it = GetInserter(elem);
      for (std::size_t i = 0; i < *dim; ++i) {
        typename Element::value_type val;
        if constexpr (1 < traits::kDimensionCount<Element>) {
          // read subdimensions
          ReadDimension(buffer, dim + 1, elem_category, categories, val);
        } else {
          buffer.ReadRaw(val, categories, elem_category);
        }
        *it++ = std::move(val);
      }
    }
  }

  void ReadDimension(FieldBuffer& buffer, DimensionConstIterator dim,
                     BufferCategory elem_category,
                     const TypeBufferCategory& categories,
                     std::vector<bool>& elem) {
    elem.resize(*dim);
    // NOLINTNEXTLINE(readability-qualified-auto)
    auto value = elem.begin();
    for (std::size_t i = 0; i < *dim; ++i) {
      bool val{false};
      buffer.ReadRaw(val, categories, elem_category);
      *value++ = val;
    }
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
    auto elem_type_oid = ElementMapping::GetOid(types);
    if (replace_oid != kInvalidOid &&
        replace_oid != ArrayMapping::GetOid(types)) {
      elem_type_oid = types.FindElementOid(replace_oid);
    }

    // Fast path for default-constructed vectors
    if (this->value.empty()) {
      io::WriteBuffer(types, buffer, static_cast<Integer>(0));  // dims
      io::WriteBuffer(types, buffer, static_cast<Integer>(0));  // flags
      io::WriteBuffer(types, buffer, static_cast<Integer>(elem_type_oid));
      return;
    }

    // Write number of dimensions
    io::WriteBuffer(types, buffer, static_cast<Integer>(dimensions));
    // Write flags
    io::WriteBuffer(types, buffer, static_cast<Integer>(0));
    // Write element type oid
    io::WriteBuffer(types, buffer, static_cast<Integer>(elem_type_oid));
    Dimensions dims = GetDimensions();
    // Write data per dimension
    WriteDimensionData(types, buffer, dims);
    // Write flat elements
    WriteData(types, dims.begin(), buffer, this->value);
  }

 private:
  template <typename Element>
  void CalculateDimensions([[maybe_unused]] DimensionIterator dim,
                           const Element& element) const {
    if constexpr (traits::kIsCompatibleContainer<Element>) {
      *dim = element.size();
      if (!element.empty()) {
        CalculateDimensions(dim + 1, *element.begin());
      }  // TODO else logic error?
    }
  }
  Dimensions GetDimensions() const {
    if constexpr (traits::kHasFixedDimensions<Container>) {
      return traits::MakeArray(
          typename traits::FixedDimensions<Container>::type{});
    } else {
      Dimensions dims{};
      CalculateDimensions(dims.begin(), this->value);
      return dims;
    }
  }
  template <typename Buffer>
  void WriteDimensionData(const UserTypes& types, Buffer& buffer,
                          const Dimensions& dims) const {
    for (auto dim : dims) {
      io::WriteBuffer(types, buffer, static_cast<Integer>(dim));
      io::WriteBuffer(types, buffer, static_cast<Integer>(1));  // lbound
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
        io::WriteRawBinary(types, buffer, sub);
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
      io::WriteRawBinary(types, buffer, sub);
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

template <typename Container>
constexpr bool EnableArrayParser() {
  if constexpr (!traits::kIsCompatibleContainer<Container>) {
    return false;
  } else {
    using ElementType = typename traits::ContainerFinalElement<Container>::type;
    return traits::kHasParser<ElementType>;
  }
}
template <typename Container>
inline constexpr bool kEnableArrayParser = EnableArrayParser<Container>();

template <typename Container>
constexpr bool EnableArrayFormatter() {
  if constexpr (!traits::kIsCompatibleContainer<Container>) {
    return false;
  } else {
    using ElementType = typename traits::ContainerFinalElement<Container>::type;
    return traits::kHasFormatter<ElementType>;
  }
}
template <typename Container>
inline constexpr bool kEnableArrayFormatter = EnableArrayFormatter<Container>();

}  // namespace detail

template <typename T>
struct CppToPg<T, std::enable_if_t<traits::detail::EnableContainerMapping<T>()>>
    : detail::ArrayPgOid<T, detail::IsElementMappedToSystem<T>()> {};

namespace traits {

template <typename T>
struct Input<T, std::enable_if_t<!detail::kCustomParserDefined<T> &&
                                 io::detail::kEnableArrayParser<T>>> {
  using type = io::detail::ArrayBinaryParser<T>;
};

template <typename T>
struct Output<T, std::enable_if_t<!detail::kCustomFormatterDefined<T> &&
                                  io::detail::kEnableArrayFormatter<T>>> {
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

// std::set
template <typename... T>
struct IsCompatibleContainer<std::set<T...>> : std::true_type {};

// std::unordered_set
template <typename... T>
struct IsCompatibleContainer<std::unordered_set<T...>> : std::true_type {};

// TODO Add more containers

}  // namespace traits

namespace detail {

/// A helper data type to write a container chunk to postgresql array buffer
/// Mimics container interface (type aliases + begin/end)
template <typename Container>
class ContainerChunk {
 public:
  static_assert(
      traits::IsCompatibleContainer<Container>{},
      "Only containers explicitly declared as compatible are supported");

  using value_type = typename Container::value_type;
  using const_iterator_type = typename Container::const_iterator;

  ContainerChunk(const_iterator_type begin, std::size_t size)
      : begin_{begin}, end_{std::next(begin, size)}, size_{size} {}

  std::size_t size() const { return size_; }
  bool empty() const { return begin_ == end_; }

  const_iterator_type begin() const { return begin_; }
  const_iterator_type cbegin() const { return begin_; }

  const_iterator_type end() const { return end_; }
  const_iterator_type cend() const { return end_; }

 private:
  const_iterator_type begin_;
  const_iterator_type end_;
  std::size_t size_;
};

/// Utility class to iterate chunks of input array
template <typename Container>
class ContainerSplitter {
 public:
  static_assert(
      traits::IsCompatibleContainer<Container>{},
      "Only containers explicitly declared as compatible are supported");

  using value_type = ContainerChunk<Container>;

  class ChunkIterator {
   public:
    using UnderlyingIterator = typename Container::const_iterator;
    ChunkIterator(const Container& container, UnderlyingIterator current,
                  std::size_t chunk_elements)
        : container_{container},
          chunk_size_{chunk_elements},
          tail_size_{
              static_cast<size_t>(std::distance(current, container_.end()))},
          current_{current} {}

    bool operator==(const ChunkIterator& rhs) const {
      return current_ == rhs.current_;
    }

    bool operator!=(const ChunkIterator& rhs) const { return !(*this == rhs); }

    value_type operator*() const { return {current_, NextStep()}; }

    ChunkIterator& operator++() {
      auto step = NextStep();
      std::advance(current_, step);
      tail_size_ -= step;
      return *this;
    }

    ChunkIterator operator++(int) {
      ChunkIterator tmp{*this};
      ++(*this);
      return tmp;
    }

    std::size_t TailSize() const { return tail_size_; }

   private:
    std::size_t NextStep() const { return std::min(chunk_size_, tail_size_); }

    const Container& container_;
    const std::size_t chunk_size_;
    std::size_t tail_size_;
    UnderlyingIterator current_;
  };

  ContainerSplitter(const Container& container, std::size_t chunk_elements)
      : container_{container}, chunk_size_{chunk_elements} {}

  std::size_t size() const {
    auto sz = container_.size();
    return sz / chunk_size_ + (sz % chunk_size_ ? 1 : 0);
  }
  bool empty() const { return container_.empty(); }

  ChunkIterator begin() const {
    return {container_, container_.begin(), chunk_size_};
  }

  ChunkIterator end() const {
    return {container_, container_.end(), chunk_size_};
  }

  std::size_t ChunkSize() const { return chunk_size_; }
  const Container& GetContainer() const { return container_; }

 private:
  const Container& container_;
  const std::size_t chunk_size_;
};

template <typename Container,
          typename Seq = std::make_index_sequence<
              boost::pfr::tuple_size_v<typename Container::value_type>>>
struct ColumnsSplitterHelper;

/// Utility helper to iterate chunks of input array in column-wise way
template <typename Container, std::size_t... Indexes>
struct ColumnsSplitterHelper<Container, std::index_sequence<Indexes...>> final {
  static_assert(sizeof...(Indexes) > 0,
                "The aggregate having 0 fields doesn't make sense");

  template <std::size_t Index>
  struct FieldProjection {
    using RowType = typename Container::value_type;
    using FieldType =
        boost::pfr::tuple_element_t<Index, typename Container::value_type>;

    const FieldType& operator()(const RowType& value) const noexcept {
      return boost::pfr::get<Index>(value);
    }
  };

  template <std::size_t Index>
  using FieldView =
      USERVER_NAMESPACE::utils::impl::ProjectingView<const Container,
                                                     FieldProjection<Index>>;

  template <typename Fn>
  static void Perform(const Container& container, std::size_t chunk_elements,
                      const Fn& fn) {
    DoSplitByChunks(chunk_elements, fn, FieldView<Indexes>{container}...);
  }

 private:
  template <typename Fn, typename... Views>
  static void DoSplitByChunks(std::size_t chunk_elements, const Fn& fn,
                              const Views&... views) {
    DoIterateByChunks(fn, ContainerSplitter{views, chunk_elements}.begin()...);
  }

  template <typename Fn, typename FirstChunkIterator,
            typename... ChunkIterators>
  static void DoIterateByChunks(const Fn& fn, FirstChunkIterator first,
                                ChunkIterators... chunks) {
    while (first.TailSize() > 0) {
      fn(*first, *chunks...);

      ++first;
      (++chunks, ...);
    }
  }
};

/// Utility class to iterate chunks of input array in column-wise way
template <typename Container>
class ContainerByColumnsSplitter final {
 public:
  ContainerByColumnsSplitter(const Container& container,
                             std::size_t chunk_elements)
      : container_{container}, chunk_elements_{chunk_elements} {}

  template <typename Fn>
  void Perform(const Fn& fn) {
    ColumnsSplitterHelper<Container>::Perform(container_, chunk_elements_, fn);
  }

 private:
  const Container& container_;
  const std::size_t chunk_elements_;
};

}  // namespace detail

namespace traits {
template <typename Container>
struct IsCompatibleContainer<io::detail::ContainerChunk<Container>>
    : IsCompatibleContainer<Container> {};

template <typename Container, typename Projection>
struct IsCompatibleContainer<
    USERVER_NAMESPACE::utils::impl::ProjectingView<const Container, Projection>>
    : IsCompatibleContainer<Container> {};
}  // namespace traits

template <typename Container>
detail::ContainerSplitter<Container> SplitContainer(
    const Container& container, std::size_t chunk_elements) {
  return {container, chunk_elements};
}

template <typename Container>
detail::ContainerByColumnsSplitter<Container> SplitContainerByColumns(
    const Container& container, std::size_t chunk_elements) {
  return {container, chunk_elements};
}

}  // namespace storages::postgres::io

USERVER_NAMESPACE_END
