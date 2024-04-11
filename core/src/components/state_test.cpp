#include <userver/utest/utest.hpp>

#include <components/component_list_test.hpp>
#include <userver/alerts/component.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/loggable_component_base.hpp>
#include <userver/components/run.hpp>
#include <userver/components/state.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/logging/component.hpp>
#include <userver/os_signals/component.hpp>
#include <userver/tracing/component.hpp>
#include <userver/utils/algo.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

void CheckTheComponents(components::State state, std::string_view where);

class Component1 final : public components::LoggableComponentBase {
 public:
  static constexpr std::string_view kName = "component-1";

  Component1(const components::ComponentConfig& config,
             const components::ComponentContext& context)
      : components::LoggableComponentBase(config, context), state_{context} {}

  void OnAllComponentsLoaded() override {
    CheckTheComponents(state_, "Component1::OnAllComponentsLoaded()");
  }

  void OnAllComponentsAreStopping() override {
    CheckTheComponents(state_, "Component1::OnAllComponentsAreStopping()");
  }

  ~Component1() override {
    CheckTheComponents(state_, "Component1::~Component1()");
  }

 private:
  components::State state_;
};

class Component2 final : public components::LoggableComponentBase {
 public:
  static constexpr std::string_view kName = "component-2";

  Component2(const components::ComponentConfig& config,
             const components::ComponentContext& context)
      : components::LoggableComponentBase(config, context), state_{context} {
    context.FindComponent<Component1>();
  }

  void OnAllComponentsLoaded() override {
    CheckTheComponents(state_, "Component2::OnAllComponentsLoaded()");
  }

  void OnAllComponentsAreStopping() override {
    CheckTheComponents(state_, "Component2::OnAllComponentsAreStopping()");
  }

  ~Component2() override {
    CheckTheComponents(state_, "Component2::~Component2()");
  }

 private:
  const components::State state_;
};

class ComponentNotLoaded final : public components::LoggableComponentBase {
 public:
  static constexpr std::string_view kName = "component-not-loaded";

  ComponentNotLoaded(const components::ComponentConfig& config,
                     const components::ComponentContext& context)
      : components::LoggableComponentBase(config, context), state_{context} {
    context.FindComponent<Component1>();
  }

  void OnAllComponentsLoaded() override {
    CheckTheComponents(state_, "ComponentNotLoaded::OnAllComponentsLoaded()");
  }

  void OnAllComponentsAreStopping() override {
    CheckTheComponents(state_,
                       "ComponentNotLoaded::OnAllComponentsAreStopping()");
  }

  ~ComponentNotLoaded() override {
    CheckTheComponents(state_, "ComponentNotLoaded::~ComponentNotLoaded()");
  }

 private:
  const components::State state_;
};

class Component3 final : public components::LoggableComponentBase {
 public:
  static constexpr std::string_view kName = "component-3";

  Component3(const components::ComponentConfig& config,
             const components::ComponentContext& context)
      : components::LoggableComponentBase(config, context), state_{context} {
    context.FindComponentOptional<Component2>();
    context.FindComponentOptional<ComponentNotLoaded>();
  }

  void OnAllComponentsLoaded() override {
    CheckTheComponents(state_, "Component3::OnAllComponentsLoaded()");
  }

  void OnAllComponentsAreStopping() override {
    CheckTheComponents(state_, "Component3::OnAllComponentsAreStopping()");
  }

  ~Component3() override {
    CheckTheComponents(state_, "Component3::~Component3()");
  }

 private:
  components::State state_;
};

void CheckTheComponents(components::State state, std::string_view where) {
  constexpr std::string_view not_existing = "not-existing_component";
  {
    const auto deps = state.GetAllDependencies(Component1::kName);
    for (auto dependency : {components::Logging::kName}) {
      EXPECT_TRUE(state.HasDependencyOn(Component1::kName, dependency))
          << where;
      EXPECT_TRUE(deps.count(dependency)) << where;
    }

    for (auto dependency :
         {Component1::kName, Component2::kName, Component3::kName,
          ComponentNotLoaded::kName, not_existing}) {
      EXPECT_FALSE(state.HasDependencyOn(Component1::kName, dependency))
          << where;
      EXPECT_FALSE(deps.count(dependency)) << where;
    }
  }

  {
    const auto deps = state.GetAllDependencies(Component2::kName);
    for (auto dependency : {components::Logging::kName, Component1::kName}) {
      EXPECT_TRUE(state.HasDependencyOn(Component2::kName, dependency))
          << where;
      EXPECT_TRUE(deps.count(dependency)) << where;
    }

    for (auto dependency : {Component2::kName, Component3::kName,
                            ComponentNotLoaded::kName, not_existing}) {
      EXPECT_FALSE(state.HasDependencyOn(Component2::kName, dependency))
          << where;
      EXPECT_FALSE(deps.count(dependency)) << where;
    }
  }

  {
    const auto deps = state.GetAllDependencies(Component3::kName);
    for (auto dependency :
         {components::Logging::kName, Component1::kName, Component2::kName}) {
      EXPECT_TRUE(state.HasDependencyOn(Component3::kName, dependency))
          << where;
      EXPECT_TRUE(deps.count(dependency)) << where;
    }

    for (auto dependency :
         {Component3::kName, ComponentNotLoaded::kName, not_existing}) {
      EXPECT_FALSE(state.HasDependencyOn(Component3::kName, dependency))
          << where;
      EXPECT_FALSE(deps.count(dependency)) << where;
    }
  }

  // For now we are conservative and UASSERT if the source component was not
  // loaded.
#ifdef NDEBUG
  EXPECT_FALSE(state.HasDependencyOn(not_existing, Component1::kName)) << where;
  EXPECT_FALSE(state.HasDependencyOn(not_existing, Component2::kName)) << where;
  EXPECT_FALSE(state.HasDependencyOn(not_existing, Component3::kName)) << where;
  EXPECT_FALSE(state.HasDependencyOn(not_existing, not_existing)) << where;
#endif
}

}  // namespace

template <>
inline constexpr auto components::kConfigFileMode<Component1> =
    ConfigFileMode::kNotRequired;

template <>
inline constexpr auto components::kConfigFileMode<Component2> =
    ConfigFileMode::kNotRequired;

template <>
inline constexpr auto components::kConfigFileMode<Component3> =
    ConfigFileMode::kNotRequired;

namespace {

constexpr std::string_view kStaticConfigBase = R"(
components_manager:
  event_thread_pool:
    threads: 1
  default_task_processor: main-task-processor
  task_processors:
    main-task-processor:
      worker_threads: 1
  components:
    component-not-loaded:
      load-enabled: false
    logging:
      fs-task-processor: main-task-processor
      loggers:
        default:
          file_path: '@null'
)";

components::ComponentList MakeComponentList() {
  return components::ComponentList()
      .Append<os_signals::ProcessorComponent>()
      .Append<components::StatisticsStorage>()
      .Append<components::Logging>()
      .Append<components::Tracer>()
      // Make sure that order in list does not affect HasDependencyOn
      .Append<Component2>()
      .Append<Component1>()
      .Append<ComponentNotLoaded>()
      .Append<Component3>()
      .Append<alerts::StorageComponent>();
}

}  // namespace

TEST_F(ComponentList, StateHasDependencyOn) {
  components::RunOnce(
      components::InMemoryConfig{std::string{kStaticConfigBase}},
      MakeComponentList());
}

USERVER_NAMESPACE_END
