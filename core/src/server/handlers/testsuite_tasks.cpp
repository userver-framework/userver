#include <userver/server/handlers/testsuite_tasks.hpp>

#include <userver/components/component.hpp>
#include <userver/logging/log.hpp>
#include <userver/testsuite/tasks.hpp>
#include <userver/tracing/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {
namespace {
const std::string kLogTaskNameTag = "testsuite_task";
const std::string kLogTaskIdTag = "testsuite_task_id";
}  // namespace

TestsuiteTasks::TestsuiteTasks(
    const components::ComponentConfig& component_config,
    const components::ComponentContext& component_context)
    : server::handlers::HttpHandlerJsonBase(component_config,
                                            component_context),
      tasks_(testsuite::GetTestsuiteTasks(component_context)) {}

formats::json::Value TestsuiteTasks::HandleRequestJsonThrow(
    [[maybe_unused]] const server::http::HttpRequest& request,
    const formats::json::Value& request_body,
    [[maybe_unused]] server::request::RequestContext& context) const {
  const auto action = request_body["action"].As<std::string>();

  if (action == "run") return ActionRun(request_body);
  if (action == "spawn") return ActionSpawn(request_body);
  if (action == "stop") return ActionStop(request_body);
  LOG_ERROR() << "Unknown action given: " << action;
  throw server::handlers::ClientError();
}

formats::json::Value TestsuiteTasks::ActionRun(
    const formats::json::Value& request_body) const {
  const auto task_name = request_body["name"].As<std::string>();
  formats::json::ValueBuilder builder(formats::json::Type::kObject);
  bool status = true;

  tracing::Span::CurrentSpan().AddTag(kLogTaskNameTag, task_name);
  LOG_INFO() << "Running testsuite task " << task_name;
  try {
    tasks_.RunTask(task_name);
  } catch (const testsuite::TaskNotFound& ex) {
    LOG_ERROR() << ex;
    throw server::handlers::ResourceNotFound(
        ExternalBody{"Task not found: " + task_name});
  } catch (const testsuite::TaskAlreadyRunning& ex) {
    LOG_ERROR() << ex;
    throw server::handlers::ClientError(
        server::handlers::HandlerErrorCode::kConflictState);
  } catch (const std::exception& exc) {
    LOG_ERROR() << "Testsuite task failed: " << exc;
    status = false;
    builder["reason"] = exc.what();
  }

  builder["status"] = status;
  return builder.ExtractValue();
}

formats::json::Value TestsuiteTasks::ActionSpawn(
    const formats::json::Value& request_body) const {
  const auto task_name = request_body["name"].As<std::string>();
  formats::json::ValueBuilder builder(formats::json::Type::kObject);
  std::string task_id;

  tracing::Span::CurrentSpan().AddTag(kLogTaskNameTag, task_name);
  LOG_INFO() << "Spawning testsuite task " << task_name;
  try {
    task_id = tasks_.SpawnTask(task_name);
  } catch (const testsuite::TaskNotFound& ex) {
    LOG_ERROR() << ex;
    throw server::handlers::ResourceNotFound();
  } catch (const testsuite::TaskAlreadyRunning& ex) {
    LOG_ERROR() << ex;
    throw server::handlers::ClientError(
        server::handlers::HandlerErrorCode::kConflictState);
  }

  tracing::Span::CurrentSpan().AddTag(kLogTaskIdTag, task_id);
  LOG_INFO() << "Testsuite task " << task_name << " spawned: " << task_id;
  builder["task_id"] = task_id;
  return builder.ExtractValue();
}

formats::json::Value TestsuiteTasks::ActionStop(
    const formats::json::Value& request_body) const {
  const auto task_id = request_body["task_id"].As<std::string>();
  formats::json::ValueBuilder builder(formats::json::Type::kObject);
  bool status = true;

  tracing::Span::CurrentSpan().AddTag(kLogTaskIdTag, task_id);
  LOG_INFO() << "Joining testsuite task " << task_id;
  try {
    tasks_.StopSpawnedTask(task_id);
  } catch (const testsuite::TaskNotFound& ex) {
    LOG_ERROR() << ex;
    throw server::handlers::ResourceNotFound();
  } catch (const testsuite::TaskAlreadyRunning& ex) {
    LOG_ERROR() << ex;
    throw server::handlers::ClientError(
        server::handlers::HandlerErrorCode::kConflictState);
  } catch (const std::exception& exc) {
    LOG_ERROR() << "Testsuite task failed: " << exc;
    status = false;
    builder["reason"] = exc.what();
  }

  LOG_INFO() << "Testsuite task " << task_id << " joined";
  builder["status"] = status;
  return builder.ExtractValue();
}

}  // namespace server::handlers

USERVER_NAMESPACE_END
