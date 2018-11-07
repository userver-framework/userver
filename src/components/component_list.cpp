#include <components/component_list.hpp>

namespace components {

ComponentList& ComponentList::AppendComponentList(ComponentList&& other) & {
  for (auto& adder : other.adders_) {
    adders_.push_back(std::move(adder));
  }
  return *this;
}

ComponentList&& ComponentList::AppendComponentList(ComponentList&& other) && {
  return std::move(AppendComponentList(std::move(other)));
}

}  // namespace components
