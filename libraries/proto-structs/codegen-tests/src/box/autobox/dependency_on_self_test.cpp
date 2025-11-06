#include <gtest/gtest.h>

#include <optional>
#include <vector>

#include <userver/utils/box.hpp>

#include <box/autobox/dependency_on_self.structs.usrv.pb.hpp>
#include <test_utils/type_assertions.hpp>

USERVER_NAMESPACE_BEGIN

TEST(BoxDependencyOnSelf, Direct) {
    using Scope = box::autobox::structs::DependencyOnSelfDirect;
    // Boost.Pfr refuses to process a struct in which first field is constructible from the same struct.
    // So we can't use Boost.Pfr to check that `Scope` has exactly 1 field.
    AssertFieldType<decltype(Scope::field), utils::Box<Scope>>();
}

TEST(BoxDependencyOnSelf, Nested) {
    using Scope = box::autobox::structs::DependencyOnSelfNested;
    AssertFieldCount<Scope::Nested, 1>();
    AssertFieldType<decltype(Scope::Nested::field), utils::Box<Scope>>();
}

TEST(BoxDependencyOnSelf, Optional) {
    using Scope = box::autobox::structs::DependencyOnSelfOptional;
    {
        // Boost.Pfr refuses to process a struct in which first field is constructible from the same struct.
        // So we can't use Boost.Pfr to check that `Scope` has exactly 1 field.
        const auto [_] = Scope{};
    }
    AssertFieldType<decltype(Scope::field), std::optional<utils::Box<Scope>>>();
}

TEST(BoxDependencyOnSelf, Repeated) {
    using Scope = box::autobox::structs::DependencyOnSelfRepeated;
    AssertFieldCount<Scope, 1>();
    AssertFieldType<decltype(Scope::field), std::vector<Scope>>();
}

TEST(BoxDependencyOnSelf, DependencyWithin) {
    using Scope = box::autobox::structs::DependencyWithinSelf;
    AssertFieldCount<Scope::B, 1>();
    AssertFieldType<decltype(Scope::B::field), Scope::A>();
}

TEST(BoxDependencyOnSelf, DependencyOnSelfCollateral) {
    using Scope = box::autobox::structs::DependencyOnSelfCollateral;

    {
        // Boost.Pfr refuses to process a struct in which first field is constructible from the same struct.
        // So we can't use Boost.Pfr to check that `Scope` has exactly 1 field.
        const auto [_] = Scope{};
    }
    AssertFieldType<decltype(Scope::field), std::optional<utils::Box<Scope>>>();

    AssertFieldCount<Scope::B, 1>();
    AssertFieldType<decltype(Scope::B::field), Scope::A>();
}
