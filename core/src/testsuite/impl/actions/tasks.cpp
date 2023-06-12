#include "tasks.hpp"

#include <userver/formats/json/value_builder.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/handlers/exceptions.hpp>
#include <userver/testsuite/tasks.hpp>
#include <userver/tracing/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite::impl::actions {
namespace {
const std::string kLogTaskNameTag = "testsuite_task";
const std::string kLogTaskIdTag = "testsuite_task_id";
}  // namespace

formats::json::Value TaskRun::Perform(
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
        server::handlers::ExternalBody{"Task not found: " + task_name});
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

formats::json::Value TaskSpawn::Perform(
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

formats::json::Value TaskStop::Perform(
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

formats::json::Value TasksList::Perform(const formats::json::Value&) const {
  formats::json::ValueBuilder names_json(formats::json::Type::kArray);
  for (const auto& name : tasks_.GetTaskNames()) {
    names_json.PushBack(name);
  }

  formats::json::ValueBuilder builder(formats::json::Type::kObject);
  builder["tasks"] = names_json.ExtractValue();
  return builder.ExtractValue();
}

}  // namespace testsuite::impl::actions

USERVER_NAMESPACE_END
