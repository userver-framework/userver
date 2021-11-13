#pragma once

/// @file userver/components/component_list.hpp
/// @brief @copybrief components::ComponentList

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include <userver/components/component_config.hpp>
#include <userver/components/manager.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

class Manager;

namespace impl {

class ComponentAdderBase {
 public:
  explicit ComponentAdderBase(std::string name) : name_(std::move(name)) {}
  virtual ~ComponentAdderBase() = default;

  const std::string& GetComponentName() const { return name_; }

  virtual void operator()(Manager&,
                          const components::ComponentConfigMap&) const = 0;

 private:
  std::string name_;
};

}  // namespace impl

/// @brief A list to keep a unique list of components to start with
/// components::Run(), utils::DaemonMain() or components::RunOnce().
class ComponentList final {
 public:
  template <typename Component>
  ComponentList& Append() &;

  template <typename Component>
  ComponentList& Append(std::string name) &;

  ComponentList& AppendComponentList(ComponentList&& other) &;
  ComponentList&& AppendComponentList(ComponentList&& other) &&;

  template <typename Component, typename... Args>
  ComponentList&& Append(Args&&...) &&;

  using Adders = std::vector<std::unique_ptr<impl::ComponentAdderBase>>;

  Adders::const_iterator begin() const { return adders_.begin(); }
  Adders::const_iterator end() const { return adders_.end(); }

  bool Contains(const std::string& name) const {
    return component_names_.count(name) > 0;
  }

 private:
  std::vector<std::unique_ptr<impl::ComponentAdderBase>> adders_;
  std::unordered_set<std::string> component_names_;
};

namespace impl {

template <typename Component>
class DefaultComponentAdder final : public ComponentAdderBase {
 public:
  DefaultComponentAdder();
  void operator()(Manager&,
                  const components::ComponentConfigMap&) const override;
};

template <typename Component>
class CustomNameComponentAdder final : public ComponentAdderBase {
 public:
  CustomNameComponentAdder(std::string name);

  void operator()(Manager&,
                  const components::ComponentConfigMap&) const override;
};

}  // namespace impl

template <typename Component>
ComponentList& ComponentList::Append() & {
  auto adder = std::make_unique<impl::DefaultComponentAdder<Component>>();
  component_names_.insert(adder->GetComponentName());
  adders_.push_back(std::move(adder));
  return *this;
}

template <typename Component>
ComponentList& ComponentList::Append(std::string name) & {
  auto adder = std::make_unique<impl::CustomNameComponentAdder<Component>>(
      std::move(name));
  component_names_.insert(adder->GetComponentName());
  adders_.push_back(std::move(adder));
  return *this;
}

template <typename Component, typename... Args>
ComponentList&& ComponentList::Append(Args&&... args) && {
  return std::move(Append<Component>(std::forward<Args>(args)...));
}

namespace impl {

template <typename Component>
DefaultComponentAdder<Component>::DefaultComponentAdder()
    : ComponentAdderBase(std::string{Component::kName}) {}

template <typename Component>
void DefaultComponentAdder<Component>::operator()(
    Manager& manager, const components::ComponentConfigMap& config_map) const {
  manager.AddComponent<Component>(config_map, GetComponentName());
}

template <typename Component>
CustomNameComponentAdder<Component>::CustomNameComponentAdder(std::string name)
    : ComponentAdderBase(std::move(name)) {}

template <typename Component>
void CustomNameComponentAdder<Component>::operator()(
    Manager& manager, const components::ComponentConfigMap& config_map) const {
  manager.AddComponent<Component>(config_map, GetComponentName());
}

}  // namespace impl
}  // namespace components

USERVER_NAMESPACE_END
