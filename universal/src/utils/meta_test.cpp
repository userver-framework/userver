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

TEST(Meta, IsInstantiationOf) {
    static_assert(meta::IsInstantiationOf<std::vector<int>, std::vector>);
    static_assert(meta::IsInstantiationOf<std::unordered_map<std::string, int>, std::unordered_map>);

    static_assert(!meta::IsInstantiationOf<int, std::vector>);
    static_assert(!meta::IsInstantiationOf<Derived, Base>);
    static_assert(!meta::IsInstantiationOf<const std::vector<int>, std::vector>);
    static_assert(!meta::IsInstantiationOf<std::vector<int>&, std::vector>);
    static_assert(!meta::IsInstantiationOf<const std::vector<int>&, std::vector>);
    static_assert(!meta::IsInstantiationOf<void, std::vector>);
}

template <typename T>
struct NonStdAllocator : std::allocator<T> {
    using std::allocator<T>::allocator;
    template <typename U>
    using rebind = NonStdAllocator<U>;
};

TEST(Meta, IsVectorLike) {
    static_assert(meta::IsVectorLike<std::vector<int>>);
    static_assert(meta::IsVectorLike<std::vector<int, NonStdAllocator<int>>>);
    static_assert(meta::IsVectorLike<std::vector<bool>>);

    static_assert(!meta::IsVectorLike<const std::vector<int>>);
    static_assert(!meta::IsVectorLike<std::vector<int>&>);
    static_assert(!meta::IsVectorLike<const std::vector<int>&>);
    static_assert(!meta::IsVectorLike<int>);
    static_assert(!meta::IsVectorLike<void>);
    static_assert(!meta::IsVectorLike<std::set<int>>);
}

struct MyRange {
    std::vector<int> impl;

    auto begin() { return impl.begin(); }
    auto end() { return impl.end(); }
};

TEST(Meta, IsRange) {
    static_assert(meta::IsRange<std::vector<int>>);
    static_assert(meta::IsRange<std::vector<bool>>);
    static_assert(meta::IsRange<std::vector<int, NonStdAllocator<int>>>);
    static_assert(meta::IsRange<std::set<int>>);
    static_assert(meta::IsRange<std::map<int, int>>);
    static_assert(meta::IsRange<std::array<int, 1>>);
    static_assert(meta::IsRange<std::string>);
    static_assert(meta::IsRange<std::wstring>);
    static_assert(meta::IsRange<int[1]>);
    static_assert(meta::IsRange<const char[42]>);
    static_assert(meta::IsRange<const std::vector<int>>);
    static_assert(meta::IsRange<std::vector<int>&>);
    static_assert(meta::IsRange<const std::vector<int>&>);
    static_assert(meta::IsRange<MyRange>);
    static_assert(meta::IsRange<boost::filesystem::path>);
    static_assert(meta::IsRange<boost::filesystem::directory_iterator>);

    static_assert(!meta::IsRange<const MyRange>);
    static_assert(!meta::IsRange<void>);
    static_assert(!meta::IsRange<int>);
    static_assert(!meta::IsRange<char*>);
    static_assert(!meta::IsRange<void>);
    static_assert(!meta::IsRange<utils::StrongTypedef<class Tag, int>>);
}

struct MyMap {
    std::vector<std::pair<int, int>> impl;

    using key_type = int;
    using mapped_type = int;

    auto begin() { return impl.begin(); }
    auto end() { return impl.end(); }
    auto at(int i) const { return impl[i]; }
};

TEST(Meta, IsMap) {
    static_assert(meta::IsMap<std::map<int, int>>);
    static_assert(meta::IsMap<std::unordered_map<int, int>>);
    static_assert(meta::IsMap<std::unordered_map<
                      int,
                      int,
                      std::hash<int>,
                      std::equal_to<>,
                      NonStdAllocator<std::pair<const int, int>>>>);
    static_assert(meta::IsMap<MyMap>);

    static_assert(!meta::IsMap<const MyMap>);
    static_assert(!meta::IsMap<void>);
    static_assert(!meta::IsMap<int>);
    static_assert(!meta::IsMap<char*>);
    static_assert(!meta::IsMap<std::vector<int>>);
    static_assert(!meta::IsMap<std::array<int, 42>>);
}

TEST(Meta, RangeValueType) {
    static_assert(std::is_same_v<meta::RangeValueType<std::vector<int>>, int>);
    static_assert(std::is_same_v<meta::RangeValueType<const std::vector<int>>, int>);
    static_assert(std::is_same_v<meta::RangeValueType<MyRange>, int>);
    static_assert(std::is_same_v<meta::RangeValueType<std::vector<bool>>, bool>);
    static_assert(std::is_same_v<meta::RangeValueType<const std::vector<bool>>, bool>);
    static_assert(std::is_same_v<meta::RangeValueType<std::map<int, int>>, std::pair<const int, int>>);
    static_assert(std::is_same_v<meta::RangeValueType<int[5]>, int>);
    static_assert(std::is_same_v<meta::RangeValueType<boost::filesystem::path>, boost::filesystem::path>);
}

TEST(Meta, IsRecursiveRange) {
    static_assert(meta::IsRecursiveRange<boost::filesystem::path>);

    static_assert(!meta::IsRecursiveRange<bool>);
    static_assert(!meta::IsRecursiveRange<std::vector<std::vector<std::vector<int>>>>);
}

TEST(Meta, IsOptional) {
    static_assert(meta::IsOptional<std::optional<bool>>);

    static_assert(!meta::IsOptional<const std::optional<int>>);
    static_assert(!meta::IsOptional<std::optional<int>&>);
    static_assert(!meta::IsOptional<const std::optional<int>&>);
    static_assert(!meta::IsOptional<boost::optional<int>>);
    static_assert(!meta::IsOptional<int>);
}

TEST(Meta, IsCharacter) {
    static_assert(meta::IsCharacter<char>);
    static_assert(meta::IsCharacter<wchar_t>);
    static_assert(meta::IsCharacter<char16_t>);
    static_assert(meta::IsCharacter<char32_t>);

    static_assert(!meta::IsCharacter<signed char>);
    static_assert(!meta::IsCharacter<unsigned char>);
    static_assert(!meta::IsCharacter<bool>);
    static_assert(!meta::IsCharacter<double>);
    static_assert(!meta::IsCharacter<void>);
    static_assert(!meta::IsCharacter<int>);
    static_assert(!meta::IsCharacter<int8_t>);
    static_assert(!meta::IsCharacter<uint8_t>);
    static_assert(!meta::IsCharacter<std::string>);
    static_assert(!meta::IsCharacter<std::string_view>);
}

TEST(Meta, IsInteger) {
    static_assert(meta::IsInteger<int>);
    static_assert(meta::IsInteger<int8_t>);
    static_assert(meta::IsInteger<uint8_t>);
    static_assert(meta::IsInteger<int16_t>);
    static_assert(meta::IsInteger<uint16_t>);
    static_assert(meta::IsInteger<int32_t>);
    static_assert(meta::IsInteger<uint32_t>);
    static_assert(meta::IsInteger<int64_t>);
    static_assert(meta::IsInteger<uint64_t>);
    static_assert(meta::IsInteger<size_t>);
    static_assert(meta::IsInteger<ptrdiff_t>);
    static_assert(meta::IsInteger<signed char>);
    static_assert(meta::IsInteger<unsigned char>);

    static_assert(!meta::IsInteger<char>);
    static_assert(!meta::IsInteger<wchar_t>);
    static_assert(!meta::IsInteger<char16_t>);
    static_assert(!meta::IsInteger<char32_t>);
    static_assert(!meta::IsInteger<bool>);
    static_assert(!meta::IsInteger<double>);
    static_assert(!meta::IsInteger<void>);
    static_assert(!meta::IsInteger<std::string>);
}

struct NonWritable {};

struct Writable {
    friend std::ostream& operator<<(std::ostream& os, [[maybe_unused]] const Writable& self) { return os; }
};

struct NonConstWritable {
    friend std::ostream& operator<<(std::ostream& os, [[maybe_unused]] NonConstWritable& self) { return os; }
};

TEST(Meta, IsOstreamWritable) {
    static_assert(meta::IsOstreamWritable<int>);
    static_assert(meta::IsOstreamWritable<double>);
    static_assert(meta::IsOstreamWritable<std::string>);
    static_assert(meta::IsOstreamWritable<Writable>);
    static_assert(meta::IsOstreamWritable<Writable&>);
    static_assert(meta::IsOstreamWritable<const Writable&>);
    static_assert(!meta::IsOstreamWritable<NonConstWritable>);
    static_assert(!meta::IsOstreamWritable<NonConstWritable&>);
    static_assert(!meta::IsOstreamWritable<const NonConstWritable&>);
    static_assert(!meta::IsOstreamWritable<NonWritable>);
    static_assert(!meta::IsOstreamWritable<NonWritable&>);
    static_assert(!meta::IsOstreamWritable<const NonWritable&>);
    static_assert(!meta::IsOstreamWritable<std::vector<int>>);
}

TEST(Meta, Sizable) {
    static_assert(meta::IsSizable<std::string>);
    static_assert(meta::IsSizable<std::string_view>);
    static_assert(meta::IsSizable<std::vector<bool>>);
    static_assert(!meta::IsSizable<int>);
    static_assert(!meta::IsSizable<void>);
}

TEST(CacheDumpMetaContainers, Reservable) {
    struct ReservableDummy {
        int* begin() { return nullptr; }
        int* end() { return nullptr; }

        std::size_t size() const { return 0; }
        void reserve(std::size_t) {}
    };

    struct NonReservableDummy {};

    static_assert(meta::IsReservable<std::vector<int>>);
    static_assert(meta::IsReservable<std::string>);
    static_assert(meta::IsReservable<ReservableDummy>);

    static_assert(!meta::IsReservable<std::set<int>>);
    static_assert(!meta::IsReservable<int>);
    static_assert(!meta::IsReservable<NonReservableDummy>);
}

TEST(Meta, IsPushBackable) {
    struct PushBackableDummy {
        int* begin() { return nullptr; }
        int* end() { return nullptr; }

        void push_back(int) {}
    };

    struct NonPushBackableDummy {};

    static_assert(meta::IsPushBackable<std::vector<int>>);
    static_assert(meta::IsPushBackable<std::string>);
    static_assert(meta::IsPushBackable<PushBackableDummy>);

    static_assert(!meta::IsPushBackable<std::array<int, 10>>);
    static_assert(!meta::IsPushBackable<std::set<int>>);
    static_assert(!meta::IsPushBackable<NonPushBackableDummy>);
}

TEST(Meta, IsFixedSizeContainer) {
    static_assert(meta::IsFixedSizeContainer<std::array<int, 10>>);

    static_assert(!meta::IsFixedSizeContainer<std::vector<int>>);
}

TEST(Meta, Inserter) {
    std::array<int, 10> array{};
    std::vector<int> vector{};
    std::set<int> set{};

    static_assert(std::is_same_v<decltype(meta::Inserter(array)), decltype(array.begin())>);
    static_assert(std::is_same_v<decltype(meta::Inserter(vector)), decltype(std::back_inserter(vector))>);
    static_assert(std::is_same_v<decltype(meta::Inserter(set)), decltype(std::inserter(set, set.end()))>);
}

USERVER_NAMESPACE_END
