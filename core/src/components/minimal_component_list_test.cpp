#include <userver/components/minimal_component_list.hpp>

#include <gtest/gtest.h>

#include <userver/components/run.hpp>
#include <userver/logging/component.hpp>

#include <components/component_list_test.hpp>

USERVER_NAMESPACE_BEGIN

TEST_F(ComponentList, Minimal) {
    components::RunOnce(components::InMemoryConfig{tests::kMinimalStaticConfig}, components::MinimalComponentList());
}

using ComponentListDeathTest = ComponentList;

TEST_F(ComponentListDeathTest, DuplicateComponentName) {
    testing::FLAGS_gtest_death_test_style = "threadsafe";

    UEXPECT_DEATH(
        components::RunOnce(
            components::InMemoryConfig{tests::kMinimalStaticConfig},
            components::MinimalComponentList()
                // Already in MinimalComponentList.
                .Append<components::Logging>()
        ),
        "Attempt to add a duplicate component 'logging'"
    );
}

USERVER_NAMESPACE_END
