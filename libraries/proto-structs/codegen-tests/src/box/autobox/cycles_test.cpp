#include <gtest/gtest.h>

#include <fmt/format.h>

#include <box/autobox/cycles.structs.usrv.pb.hpp>
#include <test_utils/type_assertions.hpp>

USERVER_NAMESPACE_BEGIN

TEST(AutoboxCycles, Self) {
    namespace ss = box::autobox::structs;
    AssertFieldType<decltype(ss::Self::self), utils::Box<ss::Self>>();
    AssertFieldCount<ss::CycleEnd, 1>();
}

TEST(AutoboxCycles, CycleLenIsThree) {
    namespace ss = box::autobox::structs;
    AssertFieldType<decltype(ss::First::c), utils::Box<ss::Second>>();
    AssertFieldType<decltype(ss::Second::c), utils::Box<ss::Third>>();
    AssertFieldType<decltype(ss::Third::c), utils::Box<ss::First>>();

    AssertFieldCount<ss::First, 1>();
    AssertFieldCount<ss::Second, 1>();
    AssertFieldCount<ss::Third, 1>();
}

TEST(AutoboxCycles, box) {
    namespace ss = box::autobox::structs;
    AssertFieldType<decltype(ss::CycleStart::cycle), utils::Box<ss::CycleEnd>>();
    AssertFieldType<decltype(ss::CycleStart::not_boxed), std::vector<ss::CycleEnd>>();
    AssertFieldType<decltype(ss::CycleEnd::cycle), utils::Box<ss::CycleStart>>();

    AssertFieldCount<ss::CycleEnd, 1>();
    AssertFieldCount<ss::CycleStart, 2>();
}

TEST(AutoboxCycles, Main) {
    namespace ss = box::autobox::structs;
    AssertFieldType<decltype(ss::Main1::inner), ss::Main1::Inner>();
    AssertFieldType<decltype(ss::Main1::Inner::cycle), utils::Box<ss::ImBelowMain1>>();
    AssertFieldType<decltype(ss::IamAboveMain2::cycle), utils::Box<ss::Main2>>();
    AssertFieldType<decltype(ss::Main2::inner), ss::Main2::Inner>();
    AssertFieldType<decltype(ss::Main2::Inner::cycle), utils::Box<ss::IamAboveMain2>>();
    AssertFieldType<decltype(ss::ImBelowMain1::cycle), utils::Box<ss::Main1>>();

    AssertFieldCount<ss::Main1, 1>();
    AssertFieldCount<ss::Main1::Inner, 1>();
    AssertFieldCount<ss::IamAboveMain2, 1>();
    AssertFieldCount<ss::Main2, 1>();
    AssertFieldCount<ss::Main2::Inner, 1>();
    AssertFieldCount<ss::ImBelowMain1, 1>();
}

TEST(AutoboxCycles, NewCycle) {
    namespace ss = box::autobox::structs;
    AssertFieldType<decltype(ss::NewCycle::inner), ss::NewCycle::Inner1>();
    AssertFieldCount<ss::NewCycle, 1>();
    AssertFieldType<decltype(ss::NewCycle::Inner1::inner), ss::NewCycle::Inner1::InnerInner>();
    AssertFieldType<decltype(ss::NewCycle::Inner1::InnerInner::inner), utils::Box<ss::NewCycle::Inner2Below>>();
    AssertFieldType<decltype(ss::NewCycle::Inner2Below::i), ss::NewCycle::Inner2Below::InnerInner>();
    AssertFieldType<decltype(ss::NewCycle::Inner2Below::InnerInner::inner), utils::Box<ss::NewCycle::Inner1>>();

    AssertFieldCount<ss::NewCycle::Inner1, 1>();
    AssertFieldCount<ss::NewCycle::Inner1::InnerInner, 1>();
    AssertFieldCount<ss::NewCycle::Inner2Below, 1>();
    AssertFieldCount<ss::NewCycle::Inner2Below::InnerInner, 1>();
}

TEST(AutoboxCycles, NewCycle2) {
    namespace ss = box::autobox::structs;
    AssertFieldType<decltype(ss::NewCycle2::inner), ss::NewCycle2::Inner1>();
    AssertFieldCount<ss::NewCycle2, 1>();
    AssertFieldType<decltype(ss::NewCycle2::Inner1::inner), ss::NewCycle2::Inner1::InnerInner>();
    AssertFieldType<decltype(ss::NewCycle2::Inner1::InnerInner::inner), utils::Box<ss::NewCycle2::Inner2Above>>();
    AssertFieldType<decltype(ss::NewCycle2::Inner2Above::i), ss::NewCycle2::Inner2Above::InnerInner>();
    AssertFieldType<decltype(ss::NewCycle2::Inner2Above::InnerInner::inner), utils::Box<ss::NewCycle2::Inner1>>();

    AssertFieldCount<ss::NewCycle2::Inner1, 1>();
    AssertFieldCount<ss::NewCycle2::Inner1::InnerInner, 1>();
    AssertFieldCount<ss::NewCycle2::Inner2Above, 1>();
    AssertFieldCount<ss::NewCycle2::Inner2Above::InnerInner, 1>();
}

TEST(AutoboxCycles, OptionalSelf) {
    namespace ss = box::autobox::structs;
    {
        // Boost.Pfr refuses to process a struct in which first field is constructible from the same struct.
        // So we can't use Boost.Pfr to check that `OptionalSelf` has exactly 1 field.
        const auto [_] = ss::OptionalSelf{};
    }
    AssertFieldType<decltype(ss::OptionalSelf::self), std::optional<utils::Box<ss::OptionalSelf>>>();
}

TEST(AutoboxCycles, OptionalDouble) {
    namespace ss = box::autobox::structs;
    AssertFieldCount<ss::OptionalDouble::First, 1>();
    AssertFieldType<decltype(ss::OptionalDouble::First::c), std::optional<utils::Box<ss::OptionalDouble::Second>>>();
    AssertFieldCount<ss::OptionalDouble::Second, 1>();
    AssertFieldType<decltype(ss::OptionalDouble::Second::c), std::optional<utils::Box<ss::OptionalDouble::First>>>();
}

TEST(AutoboxCycles, OptionalTriple) {
    namespace ss = box::autobox::structs;
    AssertFieldCount<ss::OptionalTriple::First, 1>();
    AssertFieldType<decltype(ss::OptionalTriple::First::c), std::optional<utils::Box<ss::OptionalTriple::Second>>>();
    AssertFieldCount<ss::OptionalTriple::Second, 1>();
    AssertFieldType<decltype(ss::OptionalTriple::Second::c), std::optional<utils::Box<ss::OptionalTriple::Third>>>();
    AssertFieldCount<ss::OptionalTriple::Third, 1>();
    AssertFieldType<decltype(ss::OptionalTriple::Third::c), std::optional<utils::Box<ss::OptionalTriple::First>>>();
}

USERVER_NAMESPACE_END
