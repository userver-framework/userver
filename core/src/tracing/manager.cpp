#include <userver/tracing/manager.hpp>

#include <charconv>
#include <ranges>

#include <userver/engine/task/inherited_variable.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/logging/level.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/tracing/opentelemetry.hpp>
#include <userver/tracing/tracer.hpp>
#include <userver/utils/trivial_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

namespace {

constexpr std::string_view kSampledTag = "sampled";

// The order matter for TryFillSpanBuilderFromRequest as it returns on first
// success
constexpr Format kAllFormatsOrdered[] = {
    Format::kOpenTelemetry,
    Format::kB3Alternative,
    Format::kYandexTaxi,
    Format::kYandex,
};

/// @brief Per-request data that should be available inside handlers
/// https://opentelemetry.io
struct OTelTracingHeadersInheritedData final {
    /// The W3C tracestate header value propagated from the incoming request.
    /// Empty when no tracestate was received from the upstream.
    std::string tracestate;
    /// Two-character lowercase hex representation of the W3C trace-flags byte.
    /// Defaults to "01" (sampled) when absent or unparseable in the incoming request.
    std::string traceflags_hex{"01"};
};

/// @see TracingHeadersInheritedData for details on the contents.
engine::TaskInheritedVariable<OTelTracingHeadersInheritedData> kOtelTracingHeadersInheritedData;

/// @see TracingHeadersInheritedData for details on the contents.
engine::TaskInheritedVariable<std::string> kB3TracingSampledInheritedData;

bool B3TryFillSpanBuilderFromRequest(const server::http::HttpRequest& request, tracing::SpanBuilder& span_builder) {
    namespace b3 = http::headers::b3;
    const auto& trace_id = request.GetHeader(b3::kTraceId);
    if (trace_id.empty()) {
        return false;
    }

    const auto& sampled = request.GetHeader(b3::kSampled);
    kB3TracingSampledInheritedData.Set(sampled);
    if (sampled.empty()) {
        return false;
    }

    span_builder.SetTraceId(trace_id);
    span_builder.SetParentSpanId(request.GetHeader(b3::kSpanId));
    span_builder.AddTagFrozen(std::string{kSampledTag}, sampled);
    return true;
}

template <class T>
void B3FillWithTracingContext(const tracing::Span& span, T& target) {
    namespace b3 = http::headers::b3;

    if (const auto span_id = span.GetSpanIdForChildLogs()) {
        target.SetHeader(b3::kTraceId, std::string{span.GetTraceId()});
        target.SetHeader(b3::kSpanId, std::string{*span_id});
        target.SetHeader(b3::kParentSpanId, std::string{span.GetParentId()});

        const auto* sampled = kB3TracingSampledInheritedData.GetOptional();
        if (sampled && !sampled->empty()) {
            target.SetHeader(b3::kSampled, *sampled);
        } else {
            target.SetHeader(b3::kSampled, "1");
        }
    }
}

bool OpenTelemetryTryFillSpanBuilderFromRequest(
    const server::http::HttpRequest& request,
    tracing::SpanBuilder& span_builder
) {
    namespace opentelemetry = http::headers::opentelemetry;
    const auto& traceparent = request.GetHeader(opentelemetry::kTraceParent);
    if (traceparent.empty()) {
        return false;
    }

    auto extraction_result = tracing::opentelemetry::ExtractTraceParentDataView(traceparent);
    if (!extraction_result.has_value()) {
        LOG_LIMITED_WARNING() << fmt::format(
            "Invalid traceparent header format ({}). Skipping Opentelemetry "
            "headers",
            extraction_result.error()
        );
        return false;
    }

    auto data = std::move(extraction_result).value();

    span_builder.SetTraceId(std::move(data.trace_id));
    span_builder.SetParentSpanId(std::move(data.span_id));

    SetInheritedOtelTracingData(request.GetHeader(opentelemetry::kTraceState), data.trace_flags);

    return true;
}

template <class T>
void OpenTelemetryFillWithTracingContext(const tracing::Span& span, T& target, const logging::Level log_level) {
    static constexpr std::string_view kDefaultFlagsHex{"01"};
    const auto* data = kOtelTracingHeadersInheritedData.GetOptional();
    const auto span_id = span.GetSpanIdForChildLogs();
    if (!span_id) {
        return;
    }
    const auto traceflags_hex = data ? std::string_view{data->traceflags_hex} : kDefaultFlagsHex;
    auto traceparent_result = opentelemetry::BuildTraceParentHeader(span.GetTraceId(), *span_id, traceflags_hex);

    if (!traceparent_result.has_value()) {
        LOG_LIMITED(log_level
        ) << fmt::format("Cannot build opentelemetry traceparent header ({})", traceparent_result.error());
        return;
    }

    target.SetHeader(http::headers::opentelemetry::kTraceParent, std::move(traceparent_result.value()));
    if (data && !data->tracestate.empty()) {
        target.SetHeader(http::headers::opentelemetry::kTraceState, data->tracestate);
    }
}

bool YandexTaxiTryFillSpanBuilderFromRequest(
    const server::http::HttpRequest& request,
    tracing::SpanBuilder& span_builder
) {
    const auto& trace_id = request.GetHeader(http::headers::kXYaTraceId);
    if (trace_id.empty()) {
        return false;
    }

    span_builder.SetTraceId(trace_id);

    const auto& parent_span_id = request.GetHeader(http::headers::kXYaSpanId);
    span_builder.SetParentSpanId(parent_span_id);

    const auto& parent_link = request.GetHeader(http::headers::kXYaRequestId);
    if (!parent_link.empty()) {
        span_builder.SetParentLink(parent_link);
    }

    return true;
}

template <class T>
void YandexTaxiFillWithTracingContext(const tracing::Span& span, T& target) {
    if (const auto span_id = span.GetSpanIdForChildLogs()) {
        target.SetHeader(http::headers::kXYaRequestId, std::string{span.GetLink()});
        target.SetHeader(http::headers::kXYaTraceId, std::string{span.GetTraceId()});
        target.SetHeader(http::headers::kXYaSpanId, std::string{*span_id});
    }
}

bool YandexTryFillSpanBuilderFromRequest(const server::http::HttpRequest& request, tracing::SpanBuilder& span_builder) {
    const auto& trace_id = request.GetHeader(http::headers::kXRequestId);
    if (trace_id.empty()) {
        return false;
    }

    span_builder.SetTraceId(trace_id);
    return true;
}

template <class T>
void YandexFillWithTracingContext(const tracing::Span& span, T& target) {
    target.SetHeader(http::headers::kXRequestId, std::string{span.GetTraceId()});
}

}  // namespace

OtelTraceFlags GetInheritedOtelTraceFlags() {
    static constexpr OtelTraceFlags kDefault = OtelTraceFlags::kSampled;
    const auto* data = kOtelTracingHeadersInheritedData.GetOptional();
    if (!data) {
        return kDefault;
    }
    auto result = static_cast<std::uint16_t>(kDefault);
    std::from_chars(data->traceflags_hex.data(), data->traceflags_hex.data() + data->traceflags_hex.size(), result, 16);
    return static_cast<OtelTraceFlags>(result);
}

std::string_view GetInheritedOtelTraceState() {
    const auto* data = kOtelTracingHeadersInheritedData.GetOptional();
    return data ? std::string_view{data->tracestate} : std::string_view{};
}

void SetInheritedOtelTracingData(std::string_view tracestate, std::string_view traceflags_str) {
    std::string flags_hex{"01"};
    if (!traceflags_str.empty()) {
        std::uint16_t flags = 0x01;
        std::from_chars(traceflags_str.data(), traceflags_str.data() + traceflags_str.size(), flags, 16);
        char buf[2] = {'0', '0'};
        std::to_chars(buf + (flags < 0x10u ? 1u : 0u), buf + 2, flags, 16);
        flags_hex = std::string{buf, 2};
    }
    kOtelTracingHeadersInheritedData.Set({std::string{tracestate}, std::move(flags_hex)});
}

Format FormatFromString(std::string_view format) {
    constexpr utils::TrivialBiMap kToFormat = [](auto selector) {
        return selector()
            .Case("b3-alternative", Format::kB3Alternative)
            .Case("opentelemetry", Format::kOpenTelemetry)
            .Case("taxi", Format::kYandexTaxi)
            .Case("yandex", Format::kYandex);
    };

    auto value = kToFormat.TryFind(format);
    if (!value) {
        throw std::runtime_error(
            fmt::format("Unknown tracing format '{}' (must be one of {})", format, kToFormat.DescribeFirst())
        );
    }
    return *value;
}

bool TryFillSpanBuilderFromRequest(Format format, const server::http::HttpRequest& request, SpanBuilder& span_builder) {
    switch (format) {
        case Format::kYandexTaxi:
            return YandexTaxiTryFillSpanBuilderFromRequest(request, span_builder);
        case Format::kYandex:
            return YandexTryFillSpanBuilderFromRequest(request, span_builder);
        case Format::kOpenTelemetry:
            return OpenTelemetryTryFillSpanBuilderFromRequest(request, span_builder);
        case Format::kB3Alternative:
            return B3TryFillSpanBuilderFromRequest(request, span_builder);
    }
    UINVARIANT(false, "Unexpected format of tracing headers");
}

void FillRequestWithTracingContext(Format format, const tracing::Span& span, clients::http::MiddlewareRequest request) {
    switch (format) {
        case Format::kYandexTaxi:
            YandexTaxiFillWithTracingContext(span, request);
            return;
        case Format::kYandex:
            YandexFillWithTracingContext(span, request);
            return;
        case Format::kOpenTelemetry:
            // There can be loads of false positive logs so we set up debug log lvl
            OpenTelemetryFillWithTracingContext(span, request, logging::Level::kDebug);
            return;
        case Format::kB3Alternative:
            B3FillWithTracingContext(span, request);
            return;
    }

    UINVARIANT(false, "Unexpected format of tracing headers");
}

void FillResponseWithTracingContext(Format format, const Span& span, server::http::HttpResponse& response) {
    switch (format) {
        case Format::kYandexTaxi:
            YandexTaxiFillWithTracingContext(span, response);
            return;
        case Format::kYandex:
            YandexFillWithTracingContext(span, response);
            return;
        case Format::kOpenTelemetry:
            // We can only fail to set otel header from Span here if the request did
            // not provide otel-compatible tracing headers. In this case the external
            // client will surely be satisfied with response tracing headers in the
            // original format. Thus we swallow the Span -> otel conversion error, if
            // any.
            OpenTelemetryFillWithTracingContext(span, response, logging::Level::kTrace);
            return;
        case Format::kB3Alternative:
            B3FillWithTracingContext(span, response);
            return;
    }

    UINVARIANT(false, "Unexpected format to send tracing headers");
}

bool GenericTracingManager::TryFillSpanBuilderFromRequest(
    const server::http::HttpRequest& request,
    SpanBuilder& span_builder
) const {
    bool success = false;
    bool otel_succeeded = false;

    for (const auto& format : std::views::reverse(kAllFormatsOrdered)) {
        if (in_request_response_ & format) {
            const bool ok = tracing::TryFillSpanBuilderFromRequest(format, request, span_builder);
            if (format == Format::kOpenTelemetry) {
                otel_succeeded = ok;
            }
            success |= ok;
        }
    }

    if (otel_succeeded && sampling_ == SamplingEnabled::kYes) {
        span_builder
            .SetSampled((GetInheritedOtelTraceFlags() & OtelTraceFlags::kSampled) != OtelTraceFlags::kNoTracing);
    }

    return success;
}

void GenericTracingManager::FillRequestWithTracingContext(
    const tracing::Span& span,
    clients::http::MiddlewareRequest request
) const {
    for (auto format : kAllFormatsOrdered) {
        if (new_request_ & format) {
            tracing::FillRequestWithTracingContext(format, span, request);
        }
    }
}

void GenericTracingManager::FillResponseWithTracingContext(const Span& span, server::http::HttpResponse& response)
    const {
    for (auto format : kAllFormatsOrdered) {
        if (in_request_response_ & format) {
            tracing::FillResponseWithTracingContext(format, span, response);
        }
    }
}

}  // namespace tracing

USERVER_NAMESPACE_END
