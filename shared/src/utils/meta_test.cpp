#include <userver/utils/meta.hpp>

#include <array>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>

#include <userver/utils/strong_typedef.hpp>

USERVER_NAMESPACE_BEGIN

template <typename T>
struct Base {};

struct Derived : Base<int> {};

TEST(Meta, kIsInstantiationOf) {
  static_assert(meta::kIsInstantiationOf<std::vector, std::vector<int>>);
  static_assert(meta::kIsInstantiationOf<std::unordered_map,
                                         std::unordered_map<std::string, int>>);

  static_assert(!meta::kIsInstantiationOf<std::vector, int>);
  static_assert(!meta::kIsInstantiationOf<Base, Derived>);
  static_assert(!meta::kIsInstantiationOf<std::vector, const std::vector<int>>);
  static_assert(!meta::kIsInstantiationOf<std::vector, std::vector<int>&>);
  static_assert(
      !meta::kIsInstantiationOf<std::vector, const std::vector<int>&>);
  static_assert(!meta::kIsInstantiationOf<std::vector, void>);
}

template <typename T>
struct NonStdAllocator : std::allocator<T> {
  using std::allocator<T>::allocator;
  template <typename U>
  using rebind = NonStdAllocator<U>;
};

TEST(Meta, kIsVector) {
  static_assert(meta::kIsVector<std::vector<int>>);
  static_assert(meta::kIsVector<std::vector<int, NonStdAllocator<int>>>);
  static_assert(meta::kIsVector<std::vector<bool>>);

  static_assert(!meta::kIsVector<const std::vector<int>>);
  static_assert(!meta::kIsVector<std::vector<int>&>);
  static_assert(!meta::kIsVector<const std::vector<int>&>);
  static_assert(!meta::kIsVector<int>);
  static_assert(!meta::kIsVector<void>);
  static_assert(!meta::kIsVector<std::set<int>>);
}

struct MyRange {
  std::vector<int> impl;

  auto begin() { return impl.begin(); }
  auto end() { return impl.end(); }
};

TEST(Meta, kIsRange) {
  static_assert(meta::kIsRange<std::vector<int>>);
  static_assert(meta::kIsRange<std::vector<bool>>);
  static_assert(meta::kIsRange<std::vector<int, NonStdAllocator<int>>>);
  static_assert(meta::kIsRange<std::set<int>>);
  static_assert(meta::kIsRange<std::map<int, int>>);
  static_assert(meta::kIsRange<std::array<int, 1>>);
  static_assert(meta::kIsRange<std::string>);
  static_assert(meta::kIsRange<std::wstring>);
  static_assert(meta::kIsRange<int[1]>);
  static_assert(meta::kIsRange<const char[42]>);
  static_assert(meta::kIsRange<const std::vector<int>>);
  static_assert(meta::kIsRange<std::vector<int>&>);
  static_assert(meta::kIsRange<const std::vector<int>&>);
  static_assert(meta::kIsRange<MyRange>);
  static_assert(meta::kIsRange<boost::filesystem::path>);
  static_assert(meta::kIsRange<boost::filesystem::directory_iterator>);

  static_assert(!meta::kIsRange<const MyRange>);
  static_assert(!meta::kIsRange<void>);
  static_assert(!meta::kIsRange<int>);
  static_assert(!meta::kIsRange<char*>);
  static_assert(!meta::kIsRange<void>);
  static_assert(!meta::kIsRange<utils::StrongTypedef<class Tag, int>>);
}

struct MyMap {
  std::vector<std::pair<int, int>> impl;

  using key_type = int;
  using mapped_type = int;

  auto begin() { return impl.begin(); }
  auto end() { return impl.end(); }
};

TEST(Meta, kIsMap) {
  static_assert(meta::kIsMap<std::map<int, int>>);
  static_assert(meta::kIsMap<std::unordered_map<int, int>>);
  static_assert(
      meta::kIsMap<
          std::unordered_map<int, int, std::hash<int>, std::equal_to<>,
                             NonStdAllocator<std::pair<const int, int>>>>);
  static_assert(meta::kIsMap<MyMap>);

  static_assert(!meta::kIsMap<const MyMap>);
  static_assert(!meta::kIsMap<void>);
  static_assert(!meta::kIsMap<int>);
  static_assert(!meta::kIsMap<char*>);
  static_assert(!meta::kIsMap<std::vector<int>>);
  static_assert(!meta::kIsMap<std::array<int, 42>>);
}

TEST(Meta, RangeValueType) {
  static_assert(std::is_same_v<meta::RangeValueType<std::vector<int>>, int>);
  static_assert(
      std::is_same_v<meta::RangeValueType<const std::vector<int>>, int>);
  static_assert(std::is_same_v<meta::RangeValueType<MyRange>, int>);
  static_assert(std::is_same_v<meta::RangeValueType<std::vector<bool>>, bool>);
  static_assert(
      std::is_same_v<meta::RangeValueType<const std::vector<bool>>, bool>);
  static_assert(std::is_same_v<meta::RangeValueType<std::map<int, int>>,
                               std::pair<const int, int>>);
  static_assert(std::is_same_v<meta::RangeValueType<int[5]>, int>);
  static_assert(std::is_same_v<meta::RangeValueType<boost::filesystem::path>,
                               boost::filesystem::path>);
}

TEST(Meta, kIsRecursiveRange) {
  static_assert(meta::kIsRecursiveRange<boost::filesystem::path>);

  static_assert(!meta::kIsRecursiveRange<bool>);
  static_assert(
      !meta::kIsRecursiveRange<std::vector<std::vector<std::vector<int>>>>);
}

TEST(Meta, kIsOptional) {
  static_assert(meta::kIsOptional<std::optional<bool>>);

  static_assert(!meta::kIsOptional<const std::optional<int>>);
  static_assert(!meta::kIsOptional<std::optional<int>&>);
  static_assert(!meta::kIsOptional<const std::optional<int>&>);
  static_assert(!meta::kIsOptional<boost::optional<int>>);
  static_assert(!meta::kIsOptional<int>);
}

TEST(Meta, kIsCharacter) {
  static_assert(meta::kIsCharacter<char>);
  static_assert(meta::kIsCharacter<wchar_t>);
  static_assert(meta::kIsCharacter<char16_t>);
  static_assert(meta::kIsCharacter<char32_t>);

  static_assert(!meta::kIsCharacter<signed char>);
  static_assert(!meta::kIsCharacter<unsigned char>);
  static_assert(!meta::kIsCharacter<bool>);
  static_assert(!meta::kIsCharacter<double>);
  static_assert(!meta::kIsCharacter<void>);
  static_assert(!meta::kIsCharacter<int>);
  static_assert(!meta::kIsCharacter<int8_t>);
  static_assert(!meta::kIsCharacter<uint8_t>);
  static_assert(!meta::kIsCharacter<std::string>);
  static_assert(!meta::kIsCharacter<std::string_view>);
}

TEST(Meta, kIsInteger) {
  static_assert(meta::kIsInteger<int>);
  static_assert(meta::kIsInteger<int8_t>);
  static_assert(meta::kIsInteger<uint8_t>);
  static_assert(meta::kIsInteger<int16_t>);
  static_assert(meta::kIsInteger<uint16_t>);
  static_assert(meta::kIsInteger<int32_t>);
  static_assert(meta::kIsInteger<uint32_t>);
  static_assert(meta::kIsInteger<int64_t>);
  static_assert(meta::kIsInteger<uint64_t>);
  static_assert(meta::kIsInteger<size_t>);
  static_assert(meta::kIsInteger<ptrdiff_t>);
  static_assert(meta::kIsInteger<signed char>);
  static_assert(meta::kIsInteger<unsigned char>);

  static_assert(!meta::kIsInteger<char>);
  static_assert(!meta::kIsInteger<wchar_t>);
  static_assert(!meta::kIsInteger<char16_t>);
  static_assert(!meta::kIsInteger<char32_t>);
  static_assert(!meta::kIsInteger<bool>);
  static_assert(!meta::kIsInteger<double>);
  static_assert(!meta::kIsInteger<void>);
  static_assert(!meta::kIsInteger<std::string>);
}

struct NonWritable {};

struct Writable {
  friend std::ostream& operator<<(std::ostream& os,
                                  [[maybe_unused]] const Writable& self) {
    return os;
  }
};

struct NonConstWritable {
  friend std::ostream& operator<<(std::ostream& os,
                                  [[maybe_unused]] NonConstWritable& self) {
    return os;
  }
};

TEST(Meta, kIsOstreamWritable) {
  static_assert(meta::kIsOstreamWritable<int>);
  static_assert(meta::kIsOstreamWritable<double>);
  static_assert(meta::kIsOstreamWritable<std::string>);
  static_assert(meta::kIsOstreamWritable<Writable>);
  static_assert(meta::kIsOstreamWritable<Writable&>);
  static_assert(meta::kIsOstreamWritable<const Writable&>);
  static_assert(!meta::kIsOstreamWritable<NonConstWritable>);
  static_assert(!meta::kIsOstreamWritable<NonConstWritable&>);
  static_assert(!meta::kIsOstreamWritable<const NonConstWritable&>);
  static_assert(!meta::kIsOstreamWritable<NonWritable>);
  static_assert(!meta::kIsOstreamWritable<NonWritable&>);
  static_assert(!meta::kIsOstreamWritable<const NonWritable&>);
  static_assert(!meta::kIsOstreamWritable<std::vector<int>>);
}

struct Dummy {
  using dummy_alias = int;
};

template <typename T>
using DummyAlias = typename T::dummy_alias;

TEST(Meta, Detection) {
  static_assert(meta::kIsDetected<DummyAlias, Dummy>);
  static_assert(std::is_same_v<meta::DetectedType<DummyAlias, Dummy>, int>);
  static_assert(std::is_same_v<meta::DetectedOr<bool, DummyAlias, Dummy>, int>);

  static_assert(!meta::kIsDetected<DummyAlias, std::string>);
  static_assert(std::is_same_v<meta::DetectedType<DummyAlias, std::string>,
                               meta::NotDetected>);
  static_assert(
      std::is_same_v<meta::DetectedOr<bool, DummyAlias, std::string>, bool>);
}

TEST(Meta, Sizable) {
  static_assert(meta::kIsSizable<std::string>);
  static_assert(meta::kIsSizable<std::string_view>);
  static_assert(meta::kIsSizable<std::vector<bool>>);
  static_assert(!meta::kIsSizable<int>);
  static_assert(!meta::kIsSizable<void>);
}

TEST(CacheDumpMetaContainers, Reservable) {
  struct ReservableDummy {
    void reserve(std::size_t) {}
  };

  struct NonReservableDummy {};

  static_assert(meta::kIsReservable<std::vector<int>>);
  static_assert(meta::kIsReservable<std::string>);
  static_assert(meta::kIsReservable<ReservableDummy>);

  static_assert(!meta::kIsReservable<std::set<int>>);
  static_assert(!meta::kIsReservable<int>);
  static_assert(!meta::kIsReservable<NonReservableDummy>);
}

TEST(Meta, IsPushBackable) {
  struct PushBackableDummy {
    void push_back(int) {}
  };

  struct NonPushBackableDummy {};

  static_assert(meta::kIsPushBackable<std::vector<int>>);
  static_assert(meta::kIsPushBackable<std::string>);
  static_assert(meta::kIsPushBackable<PushBackableDummy>);

  static_assert(!meta::kIsPushBackable<std::array<int, 10>>);
  static_assert(!meta::kIsPushBackable<std::set<int>>);
  static_assert(!meta::kIsPushBackable<NonPushBackableDummy>);
}

TEST(Meta, IsFixedSizeContainer) {
  static_assert(meta::kIsFixedSizeContainer<std::array<int, 10>>);

  static_assert(!meta::kIsFixedSizeContainer<std::vector<int>>);
}

TEST(Meta, Inserter) {
  std::array<int, 10> array{};
  std::vector<int> vector{};
  std::set<int> set{};

  static_assert(
      std::is_same_v<decltype(meta::Inserter(array)), decltype(array.begin())>);
  static_assert(std::is_same_v<decltype(meta::Inserter(vector)),
                               decltype(std::back_inserter(vector))>);
  static_assert(std::is_same_v<decltype(meta::Inserter(set)),
                               decltype(std::inserter(set, set.end()))>);
}

USERVER_NAMESPACE_END
