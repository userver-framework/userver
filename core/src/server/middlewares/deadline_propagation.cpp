#include <server/middlewares/deadline_propagation.hpp>

#include <server/handlers/http_server_settings.hpp>
#include <server/request/internal_request_context.hpp>

#include <userver/http/common_headers.hpp>
#include <userver/server/handlers/exceptions.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/handlers/impl/deadline_propagation_config.hpp>
#include <userver/server/http/http_error.hpp>
#include <userver/server/request/request_context.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/server/request/task_inherited_request.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/utils/from_string.hpp>
#include <userver/utils/overloaded.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {
enum class FallbackHandler;
}

namespace server::middlewares {

namespace {

std::string GetHandlerPath(const handlers::HttpHandlerBase& handler) {
  return std::string{std::visit(
      utils::Overloaded{
          [](const std::string& path) -> std::string_view { return path; },
          [](const handlers::FallbackHandler&) -> std::string_view {
            return "<fallback>";
          }},
      handler.GetConfig().path)};
}

std::optional<std::chrono::milliseconds> ParseTimeout(
    const http::HttpRequest& request) {
  const auto& timeout_ms_str = request.GetHeader(
      USERVER_NAMESPACE::http::headers::kXYaTaxiClientTimeoutMs);
  if (timeout_ms_str.empty()) return std::nullopt;

  LOG_DEBUG() << "Got client timeout_ms=" << timeout_ms_str;
  std::chrono::milliseconds timeout;
  try {
    timeout = std::chrono::milliseconds{
        utils::FromString<std::uint64_t>(timeout_ms_str)};
  } catch (const std::exception& ex) {
    LOG_LIMITED_WARNING() << "Can't parse client timeout from '"
                          << timeout_ms_str << '\'';
    return std::nullopt;
  }

  // Very large timeouts may cause overflows.
  if (timeout >= std::chrono::hours{24 * 365 * 10}) {
    LOG_LIMITED_WARNING() << "Unreasonably large timeout: " << timeout;
    return std::nullopt;
  }

  return timeout;
}

void SetFormattedErrorResponse(
    http::HttpResponse& http_response,
    handlers::FormattedErrorData&& formatted_error_data) {
  http_response.SetData(std::move(formatted_error_data.external_body));
  if (formatted_error_data.content_type) {
    http_response.SetContentType(*std::move(formatted_error_data.content_type));
  }
}

}  // namespace

struct DeadlinePropagation::RequestScope final {
  RequestScope(request::impl::InternalRequestContext& context)
      : config_snapshot{context.GetConfigSnapshot()},
        shared_dp_context{context.GetDPContext()} {}

  bool need_log_response{false};
  const dynamic_config::Snapshot& config_snapshot;
  request::impl::DeadlinePropagationContext& shared_dp_context;
};

DeadlinePropagation::DeadlinePropagation(
    const handlers::HttpHandlerBase& handler)
    : handler_{handler},
      deadline_propagation_enabled_{
          handler_.GetConfig().deadline_propagation_enabled},
      deadline_expired_status_code_{
          handler_.GetConfig().deadline_expired_status_code},
      path_{GetHandlerPath(handler_)} {}

void DeadlinePropagation::HandleRequest(
    http::HttpRequest& request, request::RequestContext& context) const {
  RequestScope dp_scope{context.GetInternalContext()};
  SetUpInheritedData(request, dp_scope);

  if (dp_scope.shared_dp_context.IsCancelledByDeadline()) {
    // We're done processing request, response is already set by us.
    return;
  }

  try {
    Next(request, context);
  } catch (const std::exception& ex) {
    // Us being here means that some middleware threw, or user-built pipeline
    // failed to process and handle a handler's exception.
    // In any case, we will apply deadline-propagation logic, and iff the
    // deadline is reached we will log the exception and swallow it, since we
    // set our own response and there's no point to let the exception fly any
    // further.
    CompleteDeadlinePropagation(request, context, dp_scope);
    if (dp_scope.shared_dp_context.IsCancelledByDeadline()) {
      // No matter what the error is we're setting our response, log and swallow
      // the exception.
      handler_.LogUnknownException(ex);
      return;
    } else {
      // Let it fly further, not our problem.
      throw;
    }
  }

  // 'Next()' succeeded, but we still have to check for deadline expiration.
  CompleteDeadlinePropagation(request, context, dp_scope);
}

void DeadlinePropagation::SetUpInheritedData(const http::HttpRequest& request,
                                             RequestScope& dp_scope) const {
  request::TaskInheritedData inherited_data{
      path_,
      request.GetMethodStr(),
      request.GetStartTime(),
      engine::Deadline{},
  };

  const utils::FastScopeGuard set_inherited_data_guard{[&]() noexcept {
    request::kTaskInheritedData.Set(std::move(inherited_data));
  }};

  SetupInheritedDeadline(request, inherited_data, dp_scope);
}

void DeadlinePropagation::SetupInheritedDeadline(
    const http::HttpRequest& request,
    request::TaskInheritedData& inherited_data, RequestScope& dp_scope) const {
  if (!deadline_propagation_enabled_) {
    return;
  }

  const auto& config_snapshot = dp_scope.config_snapshot;
  if (!config_snapshot[handlers::impl::kDeadlinePropagationEnabled]) {
    return;
  }

  const auto timeout = ParseTimeout(request);
  if (!timeout) {
    return;
  }

  dp_scope.need_log_response = config_snapshot[handlers::kLogRequest];

  auto* span_opt = tracing::Span::CurrentSpanUnchecked();
  if (span_opt) {
    span_opt->AddNonInheritableTag("deadline_received_ms", timeout->count());
  }

  const auto deadline =
      engine::Deadline::FromTimePoint(request.GetStartTime() + *timeout);
  inherited_data.deadline = deadline;

  if (deadline.IsSurelyReachedApprox()) {
    HandleDeadlineExpired(request, dp_scope,
                          "Immediate timeout (deadline propagation)");
    return;
  }

  if (config_snapshot[handlers::kCancelHandleRequestByDeadline]) {
    engine::current_task::SetDeadline(deadline);
  }
}

void DeadlinePropagation::HandleDeadlineExpired(
    const http::HttpRequest& request, RequestScope& dp_scope,
    std::string internal_message) const {
  dp_scope.shared_dp_context.SetCancelledByDeadline();
  dp_scope.shared_dp_context.SetForcedLogLevel(logging::Level::kWarning);

  auto& response = request.GetHttpResponse();
  const auto status_code = deadline_expired_status_code_;
  response.SetStatus(status_code);

  const server::http::CustomHandlerException exception_for_formatted_body{
      handlers::HandlerErrorCode::kClientError, status_code,
      handlers::ExternalBody{"Deadline expired"},
      handlers::InternalMessage{std::move(internal_message)},
      handlers::ServiceErrorCode{"deadline_expired"}};
  SetFormattedErrorResponse(response, handler_.GetFormattedExternalErrorBody(
                                          exception_for_formatted_body));

  response.SetHeader(USERVER_NAMESPACE::http::headers::kXYaTaxiDeadlineExpired,
                     "1");
}

void DeadlinePropagation::CompleteDeadlinePropagation(
    const http::HttpRequest& request, request::RequestContext& context,
    RequestScope& dp_scope) const {
  auto& response = request.GetHttpResponse();

  const auto* const inherited_data = request::kTaskInheritedData.GetOptional();
  if (!inherited_data) {
    // Handling was interrupted before it got to SetUpInheritedData.
    return;
  }

  if (!inherited_data->deadline.IsReachable()) return;

  const bool cancelled_by_deadline =
      engine::current_task::CancellationReason() ==
          engine::TaskCancellationReason::kDeadline ||
      inherited_data->deadline_signal.IsExpired() ||
      inherited_data->deadline.IsReached();

  auto* span_opt = tracing::Span::CurrentSpanUnchecked();
  if (span_opt) {
    span_opt->AddNonInheritableTag("cancelled_by_deadline",
                                   cancelled_by_deadline);
  }

  if (cancelled_by_deadline &&
      !dp_scope.shared_dp_context.IsCancelledByDeadline()) {
    const auto& original_body = response.GetData();
    if (!original_body.empty() && span_opt && span_opt->ShouldLogDefault()) {
      span_opt->AddNonInheritableTag("dp_original_body_size",
                                     original_body.size());
      if (dp_scope.need_log_response) {
        span_opt->AddNonInheritableTag(
            "dp_original_body", handler_.GetResponseDataForLoggingChecked(
                                    request, context, response.GetData()));
      }
    }
    HandleDeadlineExpired(request, dp_scope,
                          "Handling timeout (deadline propagation)");
  }
}

}  // namespace server::middlewares

USERVER_NAMESPACE_END
