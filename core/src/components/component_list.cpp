#include <userver/components/component_list.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

namespace impl {

ComponentAdderBase::ComponentAdderBase(std::string name,
                                       ConfigFileMode config_file_mode)
    : name_(std::move(name)), config_file_mode_{config_file_mode} {}

}  // namespace impl

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
