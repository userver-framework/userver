#include <tracing/jaeger.hpp>

#include <tracing/noop.hpp>

#if 0
#include <jaegertracing/Tracer.h>
#include <jaegertracing/propagation/HeadersConfig.h>
#include <jaegertracing/reporters/Reporter.h>
#include <jaegertracing/samplers/ConstSampler.h>
#endif
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include <tracing/tracing_variant.hpp>

namespace {
const std::string kStopWatchAttrName = "stopwatch_name";
const std::string kTotalTimeAttrName = "total_time";
const std::string kTimeUnitsAttrName = "stopwatch_units";
const std::string kStartTimestampAttrName = "start_timestamp";
const std::string kTraceIdName = "trace_id";
const std::string kSpanIdName = "span_id";
const std::string kParentIdName = "parent_id";

const std::string kSamplerNamePrefix = "sampler.";
}  // namespace

namespace logging {

#if 0
inline LogHelper& operator<<(LogHelper& lh, const jaegertracing::Span& span) {
  tracing::Tracer::GetTracer()->LogSpanContextTo(span, lh);
  return lh;
}
#endif

}  // namespace logging

namespace tracing {

#if 0
using RealMilliseconds = std::chrono::duration<double, std::milli>;

class LogReporter : public jaegertracing::reporters::Reporter {
 public:
  void report(const jaegertracing::Span& span) noexcept override {
    const double start_ts =
        span.startTimeSystem().time_since_epoch().count() / 1000000000.0;

    const auto start_ts_str = str(boost::format("%.6f") % start_ts);
    const auto total_time_ms =
        std::chrono::duration_cast<RealMilliseconds>(span.duration()).count();

    logging::LogExtra result({{kStopWatchAttrName, span.operationName()},
                              {kStartTimestampAttrName, start_ts_str},
                              {kTotalTimeAttrName, total_time_ms},
                              {kTimeUnitsAttrName, "ms"}});

    for (const auto& tag : span.tags()) {
      const auto& key = tag.key();

      // sampler.* is a span's private business, we're not interested in it
      if (boost::algorithm::starts_with(key, kSamplerNamePrefix)) continue;

      result.Extend(key, tracing::FromOpentracingValue(tag.value()));
    }

    LOG_INFO() << result << span;
  }

  void close() noexcept override {}
};

class JaegerTracer : public Tracer {
 public:
  JaegerTracer(std::shared_ptr<opentracing::Tracer> tracer) : Tracer(tracer) {}

  virtual void LogSpanContextTo(const opentracing::Span& span,
                                logging::LogHelper& log_helper) const override;
};

void JaegerTracer::LogSpanContextTo(const opentracing::Span& span,
                                    logging::LogHelper& log_helper) const {
  const auto& j_span = static_cast<const jaegertracing::Span&>(span);
  const auto& context = j_span.contextNoLock();

  const auto& trace_id = context.traceID();
  const auto& trace_id_str = utils::encoding::ToHexString(trace_id.high()) +
                             utils::encoding::ToHexString(trace_id.low());
  const auto& span_id_str = utils::encoding::ToHexString(context.spanID());
  const auto& parent_id_str = utils::encoding::ToHexString(context.parentID());

  logging::LogExtra result({{kTraceIdName, trace_id_str},
                            {kSpanIdName, span_id_str},
                            {kParentIdName, parent_id_str}});
  log_helper << std::move(result);
}

tracing::TracerPtr MakeJaegerLogTracer() {
  auto service_name = "uservice";
  auto sampler = std::make_shared<jaegertracing::samplers::ConstSampler>(true);
  auto reporter = std::make_shared<LogReporter>();
  auto logger = std::shared_ptr<jaegertracing::logging::Logger>(
      jaegertracing::logging::nullLogger());
  auto metrics = std::shared_ptr<jaegertracing::metrics::Metrics>(
      jaegertracing::metrics::Metrics::makeNullMetrics().release());

  auto headers = jaegertracing::propagation::HeadersConfig();
  auto options = 0;

  // TODO: full ingegration with Jaeger
#if 0
    return std::unique_ptr<CompositeReporter>(new CompositeReporter(
                { std::shared_ptr<RemoteReporter>(std::move(remoteReporter)),
                  std::make_shared<LoggingReporter>(logger) }));
#endif

  auto jaeger_tracer = jaegertracing::Tracer::make(
      service_name, sampler, reporter, logger, metrics, headers, options);
  return std::make_shared<tracing::JaegerTracer>(jaeger_tracer);
}
#endif

tracing::TracerPtr MakeJaegerLogTracer() {
  return MakeNoopTracer();
}

}  // namespace tracing
