#include <gtest/gtest.h>

#include <fmt/format.h>

#include <box/options/cycles.structs.usrv.pb.hpp>
#include <test_utils/type_assertions.hpp>

USERVER_NAMESPACE_BEGIN

TEST(OptionsCycles, Self) {
    namespace ss = box::options::structs;
    AssertFieldType<decltype(ss::Self::self), utils::Box<ss::Self>>();
    AssertFieldCount<ss::CycleEnd, 1>();
}

TEST(OptionsCycles, MyMap) {
    namespace ss = box::options::structs;

    AssertFieldType<decltype(ss::MyMap::self), utils::Box<::proto_structs::HashMap<std::string, std::string>>>();

    AssertFieldCount<ss::MyMap, 1>();
}

TEST(OptionsCycles, CycleLenIsThree) {
    namespace ss = box::options::structs;
    AssertFieldType<decltype(ss::First::c), ss::Second>();
    AssertFieldType<decltype(ss::Second::c), ss::Third>();
    AssertFieldType<decltype(ss::Third::c), utils::Box<ss::First>>();

    AssertFieldCount<ss::First, 1>();
    AssertFieldCount<ss::Second, 1>();
    AssertFieldCount<ss::Third, 1>();
}

TEST(OptionsCycles, box) {
    namespace ss = box::options::structs;
    AssertFieldType<decltype(ss::CycleStart::cycle), utils::Box<ss::CycleEnd>>();
    AssertFieldType<decltype(ss::CycleStart::not_boxed), std::vector<ss::CycleEnd>>();
    AssertFieldType<decltype(ss::CycleEnd::cycle), ss::CycleStart>();

    AssertFieldCount<ss::CycleEnd, 1>();
    AssertFieldCount<ss::CycleStart, 2>();
}

TEST(OptionsCycles, Main) {
    namespace ss = box::options::structs;
    AssertFieldType<decltype(ss::Main1::inner), ss::Main1::Inner>();
    AssertFieldType<decltype(ss::Main1::Inner::cycle), utils::Box<ss::ImBelowMain1>>();
    AssertFieldType<decltype(ss::IamAboveMain2::cycle), ss::Main2>();
    AssertFieldType<decltype(ss::Main2::inner), ss::Main2::Inner>();
    AssertFieldType<decltype(ss::Main2::Inner::cycle), utils::Box<ss::IamAboveMain2>>();
    AssertFieldType<decltype(ss::ImBelowMain1::cycle), ss::Main1>();

    AssertFieldCount<ss::Main1, 1>();
    AssertFieldCount<ss::Main1::Inner, 1>();
    AssertFieldCount<ss::IamAboveMain2, 1>();
    AssertFieldCount<ss::Main2, 1>();
    AssertFieldCount<ss::Main2::Inner, 1>();
    AssertFieldCount<ss::ImBelowMain1, 1>();
}

TEST(OptionsCycles, NewCycle) {
    namespace ss = box::options::structs;
    AssertFieldType<decltype(ss::NewCycle::inner), ss::NewCycle::Inner1>();
    AssertFieldCount<ss::NewCycle, 1>();
    AssertFieldType<decltype(ss::NewCycle::Inner1::inner), ss::NewCycle::Inner1::InnerInner>();
    AssertFieldType<decltype(ss::NewCycle::Inner1::InnerInner::inner), ss::NewCycle::Inner2Below>();
    AssertFieldType<decltype(ss::NewCycle::Inner2Below::i), ss::NewCycle::Inner2Below::InnerInner>();
    AssertFieldType<decltype(ss::NewCycle::Inner2Below::InnerInner::inner), utils::Box<ss::NewCycle::Inner1>>();

    AssertFieldCount<ss::NewCycle::Inner1, 1>();
    AssertFieldCount<ss::NewCycle::Inner1::InnerInner, 1>();
    AssertFieldCount<ss::NewCycle::Inner2Below, 1>();
    AssertFieldCount<ss::NewCycle::Inner2Below::InnerInner, 1>();
}

TEST(OptionsCycles, NewCycle2) {
    namespace ss = box::options::structs;
    AssertFieldType<decltype(ss::NewCycle2::inner), ss::NewCycle2::Inner1>();
    AssertFieldCount<ss::NewCycle2, 1>();
    AssertFieldType<decltype(ss::NewCycle2::Inner1::inner), ss::NewCycle2::Inner1::InnerInner>();
    AssertFieldType<decltype(ss::NewCycle2::Inner1::InnerInner::inner), ss::NewCycle2::Inner2Above>();
    AssertFieldType<decltype(ss::NewCycle2::Inner2Above::i), ss::NewCycle2::Inner2Above::InnerInner>();
    AssertFieldType<decltype(ss::NewCycle2::Inner2Above::InnerInner::inner), utils::Box<ss::NewCycle2::Inner1>>();

    AssertFieldCount<ss::NewCycle2::Inner1, 1>();
    AssertFieldCount<ss::NewCycle2::Inner1::InnerInner, 1>();
    AssertFieldCount<ss::NewCycle2::Inner2Above, 1>();
    AssertFieldCount<ss::NewCycle2::Inner2Above::InnerInner, 1>();
}

TEST(OptionsCycles, OptionalSelf) {
    namespace ss = box::options::structs;
    // Boost.Pfr actually refuses to process a struct, in which first field is constructible from the same struct.
#if 0
        AssertFieldCount<ss::OptionalSelf, 1>();
#endif
    AssertFieldType<decltype(ss::OptionalSelf::self), std::optional<utils::Box<ss::OptionalSelf>>>();
}

TEST(OptionsCycles, OptionalDouble) {
    namespace ss = box::options::structs;
    AssertFieldCount<ss::OptionalDouble::First, 1>();
    AssertFieldType<decltype(ss::OptionalDouble::First::c), std::optional<utils::Box<ss::OptionalDouble::Second>>>();
    AssertFieldCount<ss::OptionalDouble::Second, 1>();
    AssertFieldType<decltype(ss::OptionalDouble::Second::c), std::optional<ss::OptionalDouble::First>>();
}

TEST(OptionsCycles, OptionalTriple) {
    namespace ss = box::options::structs;
    AssertFieldCount<ss::OptionalTriple::First, 1>();
    AssertFieldType<decltype(ss::OptionalTriple::First::c), std::optional<ss::OptionalTriple::Second>>();
    AssertFieldCount<ss::OptionalTriple::Second, 1>();
    AssertFieldType<decltype(ss::OptionalTriple::Second::c), std::optional<ss::OptionalTriple::Third>>();
    AssertFieldCount<ss::OptionalTriple::Third, 1>();
    AssertFieldType<decltype(ss::OptionalTriple::Third::c), std::optional<utils::Box<ss::OptionalTriple::First>>>();
}

USERVER_NAMESPACE_END
