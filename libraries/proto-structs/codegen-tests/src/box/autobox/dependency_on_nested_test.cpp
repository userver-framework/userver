#include <gtest/gtest.h>

#include <vector>

#include <userver/utils/box.hpp>

#include <box/autobox/dependency_on_nested.structs.usrv.pb.hpp>
#include <test_utils/type_assertions.hpp>

USERVER_NAMESPACE_BEGIN

TEST(BoxDependencyOnNested, NonCyclic) {
    using Scope = box::autobox::structs::DependencyOnNested;
    AssertFieldCount<Scope::Foo, 1>();
    AssertFieldType<decltype(Scope::Foo::field), Scope::Bar::Nested>();
}

TEST(BoxDependencyOnNested, NonCyclicDoublyNested) {
    using Scope = box::autobox::structs::DependencyOnNestedNested;
    AssertFieldCount<Scope::Foo, 1>();
    AssertFieldType<decltype(Scope::Foo::field), Scope::Bar::Nested::Nested2>();
}

TEST(BoxDependencyOnNested, RepeatedNonCyclic) {
    using Scope = box::autobox::structs::RepeatedDependencyOnNested;
    AssertFieldCount<Scope::Foo, 1>();
    AssertFieldType<decltype(Scope::Foo::field), std::vector<Scope::Bar::Nested>>();
}

TEST(BoxDependencyOnNested, Cyclic) {
    using Scope = box::autobox::structs::CyclicDependencyOnNested;

    AssertFieldCount<Scope::Foo, 1>();
    AssertFieldType<decltype(Scope::Foo::field), Scope::Bar::Nested>();

    AssertFieldCount<Scope::Bar, 1>();
    AssertFieldType<decltype(Scope::Bar::field), utils::Box<Scope::Foo>>();
}

TEST(BoxDependencyOnNested, RepeatedCyclic) {
    using Scope = box::autobox::structs::RepeatedCyclicDependencyOnNested;

    AssertFieldCount<Scope::Foo, 1>();
    AssertFieldType<decltype(Scope::Foo::field), std::vector<Scope::Bar::Nested>>();

    AssertFieldCount<Scope::Bar, 1>();
    AssertFieldType<decltype(Scope::Bar::field), utils::Box<Scope::Foo>>();
}

TEST(BoxDependencyOnNested, VectorsDoNotCreateCycles) {
    using Scope = box::autobox::structs::VectorsDoNotCreateCycles;

    AssertFieldCount<Scope::Foo, 1>();
    AssertFieldType<decltype(Scope::Foo::field), std::vector<Scope::Bar>>();

    AssertFieldCount<Scope::Bar, 1>();
    AssertFieldType<decltype(Scope::Bar::field), std::vector<Scope::Foo>>();
}
