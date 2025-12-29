#include <userver/utest/utest.hpp>

#include <components/component_list_test.hpp>
#include <userver/components/component_base.hpp>
#include <userver/components/container.hpp>
#include <userver/components/minimal_component_list.hpp>
#include <userver/components/run.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/logging/component.hpp>

#include <userver/utest/using_namespace_userver.hpp>  // only for tests

namespace {

/// [definition]
// A simple containerized type
struct X {
    // Default ctr => the container creates `X` with no external dependencies
    X() = default;
    X(const X&) = delete;
};

// Register a container with name "x" that stores an object of type X
constexpr std::string_view ContainerName(components::Of<X>) { return "x"; }

// A containerized type that has a dependency of type `X`
struct Y {
    // Container<Y> will use a dependency of type `X`.
    // `X` is a containerized type, so it will be searched by
    // `FindComponent<Container<X>>().Get()`.
    Y(X&) {}
};

// Register a container with name "y" that stores an object of type Y
constexpr std::string_view ContainerName(components::Of<Y>) { return "y"; }
/// [definition]

struct UsingDynamicConfig {
    explicit UsingDynamicConfig(USERVER_NAMESPACE::dynamic_config::Source) {}
};

constexpr std::string_view ContainerName(components::Of<UsingDynamicConfig>) { return "udc"; }

struct MyConfig {
    int value{};
};

MyConfig Parse(const yaml_config::YamlConfig& value, formats::parse::To<MyConfig>)
{
    MyConfig cfg;
    cfg.value = value["value"].As<int>(123);
    return cfg;
}

struct MyStruct {
    MyStruct(const MyConfig&, const X&) {}
};

constexpr std::string_view ContainerName(components::Of<MyStruct>) { return "with-config"; }

}  // namespace

TEST(ComponentsContainer, NoDependencies)
{
    components::RunOnce(
        components::InMemoryConfig{std::string{tests::kMinimalStaticConfig}},
        components::MinimalComponentList().Append<components::Container<X>>()
    );
}

TEST(ComponentsContainer, OneDependency)
{
    /// [registration]
    // Register all containerized types in the component system as `Container<T>`
    auto component_list =
        components::MinimalComponentList()  //
            .Append<components::Container<X>>()
            .Append<components::Container<Y>>();
    /// [registration]
    components::RunOnce(components::InMemoryConfig{std::string{tests::kMinimalStaticConfig}}, component_list);
}

TEST(ComponentsContainer, DynamicConfigSource)
{
    components::RunOnce(
        components::InMemoryConfig{std::string{tests::kMinimalStaticConfig}},
        components::MinimalComponentList().Append<components::Container<UsingDynamicConfig>>()
    );
}

TEST(ComponentsContainer, WithConfig)
{
    components::RunOnce(
        components::InMemoryConfig{std::string{tests::kMinimalStaticConfig}},
        components::MinimalComponentList().Append<components::Container<X>>().Append<components::Container<MyStruct>>()
    );
}
