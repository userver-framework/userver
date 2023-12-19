#include <userver/components/component_list.hpp>

#include <userver/utils/assert.hpp>

#include <components/manager.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

namespace impl {

void AddComponentImpl(Manager& manager,
                      const components::ComponentConfigMap& config_map,
                      const std::string& name, ComponentBaseFactory factory) {
  manager.AddComponentImpl(config_map, name, std::move(factory));
}

ComponentAdderBase::ComponentAdderBase(std::string name,
                                       ConfigFileMode config_file_mode)
    : name_(std::move(name)), config_file_mode_{config_file_mode} {}

ComponentAdderBase::~ComponentAdderBase() = default;
}  // namespace impl

ComponentList& ComponentList::AppendComponentList(ComponentList&& other) & {
  adders_.merge(other.adders_);
  UINVARIANT(other.adders_.empty(),
             fmt::format("Duplicate component '{}' in `*this` and `other`",
                         (*other.adders_.begin())->GetComponentName()));
  return *this;
}

ComponentList&& ComponentList::AppendComponentList(ComponentList&& other) && {
  return std::move(AppendComponentList(std::move(other)));
}

yaml_config::Schema ComponentList::GetStaticConfigSchema() const {
  auto schema = yaml_config::Schema::EmptyObject();
  schema.properties.emplace();
  for (const auto& adder : adders_) {
    schema.properties->emplace(
        adder->GetComponentName(),
        yaml_config::SchemaPtr(adder->GetStaticConfigSchema()));
  }
  return schema;
}

ComponentList& ComponentList::Append(impl::ComponentAdderPtr&& added) & {
  auto [it, ok] = adders_.insert(std::move(added));
  UINVARIANT(ok, fmt::format("Attempt to add a duplicate component '{}'",
                             added->GetComponentName()));
  return *this;
}

}  // namespace components

USERVER_NAMESPACE_END
