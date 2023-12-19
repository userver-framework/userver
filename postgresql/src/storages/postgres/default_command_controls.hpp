#pragma once

#include <atomic>
#include <memory>
#include <string>

#include <userver/rcu/rcu.hpp>

#include <storages/postgres/detail/pg_impl_types.hpp>
#include <userver/storages/postgres/options.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

// Stores `CommandControl`s which could be used by default in `Connection` if no
// user's command control was specified for a request.
// It can be default cmd ctl for all requests or for requests from a specific
// http-handler.
class DefaultCommandControls {
 public:
  DefaultCommandControls(
      const CommandControl& default_cmd_ctl_src,
      CommandControlByHandlerMap handlers_command_control_src,
      CommandControlByQueryMap queries_command_control);

  CommandControl GetDefaultCmdCtl() const;

  void UpdateDefaultCmdCtl(
      const CommandControl& default_cmd_ctl,
      detail::DefaultCommandControlSource source =
          detail::DefaultCommandControlSource::kGlobalConfig);

  OptionalCommandControl GetHandlerCmdCtl(std::string_view path,
                                          std::string_view method) const;

  OptionalCommandControl GetQueryCmdCtl(const std::string& query_name) const;

  void UpdateHandlersCommandControl(
      CommandControlByHandlerMap&& handlers_command_control);

  void UpdateQueriesCommandControl(
      CommandControlByQueryMap&& queries_command_control);

 private:
  struct Data {
    Data(const CommandControl& default_cmd_ctl_src,
         CommandControlByHandlerMap&& handlers_command_control_src,
         CommandControlByQueryMap&& queries_command_control_src);

    rcu::Variable<CommandControl> default_cmd_ctl;
    rcu::Variable<CommandControlByHandlerMap> handlers_command_control{};
    rcu::Variable<CommandControlByQueryMap> queries_command_control{};
    std::atomic<bool> has_user_default_cc{false};
  };

  std::shared_ptr<Data> data_;
};

}  // namespace storages::postgres

USERVER_NAMESPACE_END
