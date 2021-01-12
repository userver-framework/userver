#pragma once

#include <atomic>
#include <memory>
#include <string>

#include <rcu/rcu.hpp>

#include <storages/postgres/detail/pg_impl_types.hpp>
#include <storages/postgres/options.hpp>

namespace storages::postgres {

// Stores `CommandControl`s which could be used by default in `Connection` if no
// user's command control was specified for a request.
// It can be default cmd ctl for all requests or for requests from a specific
// http-handler.
class DefaultCommandControls {
 public:
  DefaultCommandControls(
      const CommandControl& default_cmd_ctl_src,
      CommandControlByHandlerMap handlers_command_control_src);

  CommandControl GetDefaultCmdCtl() const;

  void UpdateDefaultCmdCtl(
      const CommandControl& default_cmd_ctl,
      detail::DefaultCommandControlSource source =
          detail::DefaultCommandControlSource::kGlobalConfig);

  OptionalCommandControl GetHandlerCmdCtl(const std::string& path,
                                          const std::string& method) const;

  void UpdateHandlersCommandControl(
      const CommandControlByHandlerMap& handlers_command_control);

 private:
  struct Data {
    Data(const CommandControl& default_cmd_ctl_src,
         CommandControlByHandlerMap handlers_command_control_src);

    rcu::Variable<CommandControl> default_cmd_ctl{};
    rcu::Variable<CommandControlByHandlerMap> handlers_command_control{};
    std::atomic<bool> has_user_default_cc{false};
  };

  std::shared_ptr<Data> data_;
};

}  // namespace storages::postgres
