#include <userver/components/component_list.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

ComponentList& ComponentList::AppendComponentList(ComponentList&& other) & {
  for (auto& adder : other.adders_) {
    component_names_.insert(adder->GetComponentName());
    adders_.push_back(std::move(adder));
  }
  return *this;
}

ComponentList&& ComponentList::AppendComponentList(ComponentList&& other) && {
  return std::move(AppendComponentList(std::move(other)));
}

}  // namespace components

USERVER_NAMESPACE_END
