#include <gtest/gtest.h>

#include <concepts>

#include <fmt/format.h>
#include <boost/pfr.hpp>

#include <userver/proto-structs/unbreakable_dependency_cycle.hpp>

#include <box/autobox/unbreakable_cycle.structs.usrv.pb.hpp>

USERVER_NAMESPACE_BEGIN

TEST(AutoboxUnbreakableCycle, SimpleCycle) {
    namespace ss = box::autobox::structs;

    static_assert(boost::pfr::tuple_size<ss::UnbreakableCycle::A>() == 1);
    static_assert(std::same_as<decltype(ss::UnbreakableCycle::A::b), proto_structs::UnbreakableDependencyCycle>);

    static_assert(boost::pfr::tuple_size<ss::UnbreakableCycle::B>() == 1);
    static_assert(std::same_as<decltype(ss::UnbreakableCycle::B::a), proto_structs::UnbreakableDependencyCycle>);
}

USERVER_NAMESPACE_END
