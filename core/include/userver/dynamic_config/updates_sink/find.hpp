#pragma once

/// @file userver/dynamic_config/updates_sink/find.hpp
/// @brief Function for retrieving dynamic config updates sink specified in the
/// static config.

#include <type_traits>

#include <userver/components/component_fwd.hpp>
#include <userver/dynamic_config/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config {

namespace impl {

components::DynamicConfigUpdatesSinkBase& FindUpdatesSink(
    const components::ComponentConfig&, const components::ComponentContext&);

inline bool has_updater = true;

template <typename Dummy = void>
struct RegisterUpdater {
  static_assert(std::is_same_v<Dummy, void>);
  static inline const bool done = ((has_updater = true), true);
};

}  // namespace impl

/// @brief Returns component to which incoming dynamic config updates should be
/// forwarded.
///
/// Component to be used as an updates sink is determined by the `updates-sink`
/// static config field. If this field is not set, then
/// components::DynamicConfig is used as a default sink.
///
/// @warning Can only be called from other component's constructor in a task
/// where that constructor was called. May block and asynchronously wait for the
/// creation of the requested component.
///
/// @note It is illegal to use the same updates sink from several components.
template <typename Dummy = void>
components::DynamicConfigUpdatesSinkBase& FindUpdatesSink(
    const components::ComponentConfig& config,
    const components::ComponentContext& context) {
  (void)impl::RegisterUpdater<Dummy>::done;
  return impl::FindUpdatesSink(config, context);
}

}  // namespace dynamic_config

USERVER_NAMESPACE_END
