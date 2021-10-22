#include <storages/postgres/default_command_controls.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

DefaultCommandControls::DefaultCommandControls(
    const CommandControl& default_cmd_ctl_src,
    CommandControlByHandlerMap handlers_command_control_src,
    CommandControlByQueryMap queries_command_control)
    : data_(std::make_shared<Data>(default_cmd_ctl_src,
                                   std::move(handlers_command_control_src),
                                   std::move(queries_command_control))) {}

CommandControl DefaultCommandControls::GetDefaultCmdCtl() const {
  UASSERT(data_);
  return data_->default_cmd_ctl.ReadCopy();
}

void DefaultCommandControls::UpdateDefaultCmdCtl(
    const CommandControl& cmd_ctl, detail::DefaultCommandControlSource source) {
  UASSERT(data_);
  {
    if (source == detail::DefaultCommandControlSource::kGlobalConfig &&
        data_->has_user_default_cc)
      return;
    auto reader = data_->default_cmd_ctl.Read();
    if (*reader == cmd_ctl) return;
  }
  auto writer = data_->default_cmd_ctl.StartWrite();
  // source must be checked under lock to avoid races
  switch (source) {
    case detail::DefaultCommandControlSource::kGlobalConfig:
      if (data_->has_user_default_cc) return;
      break;
    case detail::DefaultCommandControlSource::kUser:
      data_->has_user_default_cc = true;
      break;
  }
  *writer = cmd_ctl;
  writer.Commit();
}

OptionalCommandControl DefaultCommandControls::GetHandlerCmdCtl(
    const std::string& path, const std::string& method) const {
  UASSERT(data_);
  auto reader = data_->handlers_command_control.Read();
  return GetHandlerOptionalCommandControl(*reader, path, method);
}

OptionalCommandControl DefaultCommandControls::GetQueryCmdCtl(
    const std::string& query_name) const {
  UASSERT(data_);
  auto reader = data_->queries_command_control.Read();
  return GetQueryOptionalCommandControl(*reader, query_name);
}

void DefaultCommandControls::UpdateHandlersCommandControl(
    const CommandControlByHandlerMap& handlers_command_control) {
  UASSERT(data_);
  {
    auto reader = data_->handlers_command_control.Read();
    if (*reader == handlers_command_control) return;
  }
  data_->handlers_command_control.Assign(handlers_command_control);
}

void DefaultCommandControls::UpdateQueriesCommandControl(
    const CommandControlByQueryMap& queries_command_control) {
  UASSERT(data_);
  {
    auto reader = data_->queries_command_control.Read();
    if (*reader == queries_command_control) return;
  }
  data_->queries_command_control.Assign(queries_command_control);
}

DefaultCommandControls::Data::Data(
    const CommandControl& default_cmd_ctl_src,
    CommandControlByHandlerMap handlers_command_control_src,
    CommandControlByQueryMap queries_command_control_src)
    : default_cmd_ctl(default_cmd_ctl_src),
      handlers_command_control(std::move(handlers_command_control_src)),
      queries_command_control(std::move(queries_command_control_src)) {}

}  // namespace storages::postgres

USERVER_NAMESPACE_END
