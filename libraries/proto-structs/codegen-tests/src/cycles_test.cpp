#include <gtest/gtest.h>

#include <fmt/format.h>
#include <boost/pfr.hpp>

#include <simple/cycles.structs.usrv.pb.hpp>

USERVER_NAMESPACE_BEGIN

template <typename Field, typename Expected>
constexpr void AssertFieldType() {
    static_assert(std::is_same_v<Field, Expected>, "Must be equal");
}

template <typename T>
constexpr std::size_t GetFieldCount() {
    return boost::pfr::tuple_size<T>::value;
}

template <typename T, std::size_t Count>
constexpr void AssertFieldCount() {
    static_assert(GetFieldCount<T>() == Count);
}

TEST(Cycles, Self) {
    namespace ss = simple::structs;
    AssertFieldType<decltype(ss::Self::self), USERVER_NAMESPACE::utils::Box<ss::Self>>();
    AssertFieldCount<ss::CycleEnd, 1>();
}

TEST(Cycles, MyMap) {
    namespace ss = simple::structs;

    AssertFieldType<
        decltype(ss::MyMap::self),
        USERVER_NAMESPACE::utils::Box<::proto_structs::HashMap<std::string, std::string>>>();

    AssertFieldCount<ss::MyMap, 1>();
}

TEST(Cycles, CycleLenIsThree) {
    namespace ss = simple::structs;
    AssertFieldType<decltype(ss::First::c), std::optional<ss::Second>>();
    AssertFieldType<decltype(ss::Second::c), std::optional<ss::Third>>();
    AssertFieldType<decltype(ss::Third::c), USERVER_NAMESPACE::utils::Box<ss::First>>();

    AssertFieldCount<ss::First, 1>();
    AssertFieldCount<ss::Second, 1>();
    AssertFieldCount<ss::Third, 1>();
}

TEST(Cycles, Simple) {
    namespace ss = simple::structs;
    AssertFieldType<decltype(ss::CycleStart::cycle), USERVER_NAMESPACE::utils::Box<ss::CycleEnd>>();
    AssertFieldType<decltype(ss::CycleStart::not_boxed), std::vector<ss::CycleEnd>>();
    AssertFieldType<decltype(ss::CycleEnd::cycle), std::optional<ss::CycleStart>>();

    AssertFieldCount<ss::CycleEnd, 1>();
    AssertFieldCount<ss::CycleStart, 2>();
}

TEST(Cycles, Main) {
    namespace ss = simple::structs;
    AssertFieldType<decltype(ss::Main1::inner), std::optional<ss::Main1::Inner>>();
    AssertFieldType<decltype(ss::Main1::Inner::cycle), USERVER_NAMESPACE::utils::Box<ss::ImBelowMain1>>();
    AssertFieldType<decltype(ss::IamAboveMain2::cycle), std::optional<ss::Main2>>();
    AssertFieldType<decltype(ss::Main2::inner), std::optional<ss::Main2::Inner>>();
    AssertFieldType<decltype(ss::Main2::Inner::cycle), USERVER_NAMESPACE::utils::Box<ss::IamAboveMain2>>();
    AssertFieldType<decltype(ss::ImBelowMain1::cycle), std::optional<ss::Main1>>();

    AssertFieldCount<ss::Main1, 1>();
    AssertFieldCount<ss::Main1::Inner, 1>();
    AssertFieldCount<ss::IamAboveMain2, 1>();
    AssertFieldCount<ss::Main2, 1>();
    AssertFieldCount<ss::Main2::Inner, 1>();
    AssertFieldCount<ss::ImBelowMain1, 1>();
}

TEST(Cycles, NewCycle) {
    namespace ss = simple::structs;
    AssertFieldType<decltype(ss::NewCycle::inner), std::optional<ss::NewCycle::Inner1>>();
    AssertFieldCount<ss::NewCycle, 1>();
    AssertFieldType<decltype(ss::NewCycle::Inner1::inner), std::optional<ss::NewCycle::Inner1::InnerInner>>();
    AssertFieldType<decltype(ss::NewCycle::Inner1::InnerInner::inner), std::optional<ss::NewCycle::Inner2Below>>();
    AssertFieldType<decltype(ss::NewCycle::Inner2Below::i), std::optional<ss::NewCycle::Inner2Below::InnerInner>>();
    AssertFieldType<
        decltype(ss::NewCycle::Inner2Below::InnerInner::inner),
        USERVER_NAMESPACE::utils::Box<ss::NewCycle::Inner1>>();

    AssertFieldCount<ss::NewCycle::Inner1, 1>();
    AssertFieldCount<ss::NewCycle::Inner1::InnerInner, 1>();
    AssertFieldCount<ss::NewCycle::Inner2Below, 1>();
    AssertFieldCount<ss::NewCycle::Inner2Below::InnerInner, 1>();
}

TEST(Cycles, NewCycle2) {
    namespace ss = simple::structs;
    AssertFieldType<decltype(ss::NewCycle2::inner), std::optional<ss::NewCycle2::Inner1>>();
    AssertFieldCount<ss::NewCycle2, 1>();
    AssertFieldType<decltype(ss::NewCycle2::Inner1::inner), std::optional<ss::NewCycle2::Inner1::InnerInner>>();
    AssertFieldType<decltype(ss::NewCycle2::Inner1::InnerInner::inner), std::optional<ss::NewCycle2::Inner2Above>>();
    AssertFieldType<decltype(ss::NewCycle2::Inner2Above::i), std::optional<ss::NewCycle2::Inner2Above::InnerInner>>();
    AssertFieldType<
        decltype(ss::NewCycle2::Inner2Above::InnerInner::inner),
        USERVER_NAMESPACE::utils::Box<ss::NewCycle2::Inner1>>();

    AssertFieldCount<ss::NewCycle2::Inner1, 1>();
    AssertFieldCount<ss::NewCycle2::Inner1::InnerInner, 1>();
    AssertFieldCount<ss::NewCycle2::Inner2Above, 1>();
    AssertFieldCount<ss::NewCycle2::Inner2Above::InnerInner, 1>();
}

USERVER_NAMESPACE_END
