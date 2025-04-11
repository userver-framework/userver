#include <userver/components/minimal_component_list.hpp>

#include <gtest/gtest.h>

#include <userver/components/run.hpp>

#include <components/component_list_test.hpp>

USERVER_NAMESPACE_BEGIN

TEST_F(ComponentList, Minimal) {
    components::RunOnce(components::InMemoryConfig{tests::kMinimalStaticConfig}, components::MinimalComponentList());
}

USERVER_NAMESPACE_END
