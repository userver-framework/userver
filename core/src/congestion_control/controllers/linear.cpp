#include <userver/congestion_control/controllers/linear.hpp>

#include <userver/logging/log.hpp>
#include <userver/utils/impl/userver_experiments.hpp>

USERVER_NAMESPACE_BEGIN

namespace congestion_control::v2 {

namespace {
constexpr std::size_t kCurrentLoadEpochs = 3;

Sensor::SingleObjectData MergeIntoSingleObjectData(
    const std::unordered_map<std::string, Sensor::SingleObjectData>& objects
) {
    Sensor::SingleObjectData result;
    for (const auto& [_, object_stats] : objects) {
        result.timings_sum_ms += object_stats.timings_sum_ms;
        result.total += object_stats.total;
        result.timeouts += object_stats.timeouts;
    }
    return result;
}

}  // namespace

LinearController::LinearController(
    const std::string& name,
    v2::Sensor& sensor,
    Limiter& limiter,
    Stats& stats,
    const StaticConfig& config,
    dynamic_config::Source config_source,
    std::function<v2::Config(const dynamic_config::Snapshot&)> config_getter
)
    : Controller(name, sensor, limiter, stats, {config.fake_mode, config.enabled}),
      config_(config),
      current_load_(kCurrentLoadEpochs),
      config_source_(config_source),
      config_getter_(std::move(config_getter)) {}

Controller::LimitWithDetails LinearController::Update(const Sensor::Data& current) {
    if (current.objects.empty()) {
        return {Limit{}, std::nullopt};
    }
    auto dyn_config = config_source_.GetSnapshot();
    v2::Config config = config_getter_(dyn_config);
    bool overloaded = false;
    std::string cc_details;
    current_load_.Update(current.current_load);
    auto current_load = current_load_.GetSmoothed();

    const auto& objects =
        (config.use_separate_stats
             ? current.objects
             : std::unordered_map<std::string, Sensor::SingleObjectData>{
                   {Sensor::SingleObjectData::kCommonObjectName, MergeIntoSingleObjectData(current.objects)}});

    for (const auto& [object_name, object_stats] : objects) {
        auto rate = object_stats.GetRate();
        auto& current_object_timings = separate_timings_[object_name];
        auto& short_timings = current_object_timings.short_timings;
        auto& long_timings = current_object_timings.long_timings;

        auto timings_avg_ms = object_stats.timings_sum_ms / object_stats.total;

        short_timings.Update(timings_avg_ms);

        bool object_overloaded = 100 * rate > config.errors_threshold_percent;

        const std::size_t divisor = std::max<std::size_t>(long_timings.GetSmoothed(), config.min_timings.count());

        std::string sensor_string;
        if (config.use_separate_stats) {
            sensor_string =
                fmt::format("{} current_load={} object_name={}", object_stats.ToLogString(), current_load, object_name);
        } else {
            sensor_string = fmt::format("{} current_load={}", object_stats.ToLogString(), current_load);
            cc_details = sensor_string;
        }

        LOG_DEBUG() << "CC mongo:"
                    << " sensor=(" << sensor_string << ") divisor=" << divisor
                    << " short_timings_.GetMinimal()=" << short_timings.GetMinimal()
                    << " long_timings_.GetSmoothed()=" << long_timings.GetSmoothed();

        if (object_stats.total < config.min_qps && !current_limit_) {
            // Too little QPS, timings avg data is VERY noisy, EPS is noisy
            continue;
        }

        if (epochs_passed_ < kLongTimingsEpochs) {
            // First seconds of service life might be too noisy
            epochs_passed_++;
            long_timings.Update(timings_avg_ms);
            continue;
        }

        if (static_cast<std::size_t>(short_timings.GetMinimal()) > config.timings_burst_threshold * divisor) {
            // Do not update long_timings_, it is sticky to "good" timings
            object_overloaded = true;
            cc_details = sensor_string;
        } else {
            long_timings.Update(timings_avg_ms);
        }
        overloaded = overloaded || object_overloaded;
    }

    if (overloaded) {
        if (current_limit_) {
            *current_limit_ = *current_limit_ * 0.95;
        } else {
            LOG_ERROR() << GetName() << " Congestion Control is activated";
            current_limit_ = current_load;
        }
    } else {
        if (current_limit_) {
            if (current_limit_ > current_load + config.safe_delta_limit) {
                // TODO: several seconds in a row
                if (current_limit_.has_value()) {
                    LOG_ERROR() << GetName() << " Congestion Control is deactivated";
                }

                current_limit_.reset();
            } else {
                ++*current_limit_;
            }
        }
    }

    if (current_limit_.has_value() && current_limit_ < config.min_limit) {
        current_limit_ = config.min_limit;
    }
    return {{current_limit_, current.current_load}, cc_details};
}

LinearController::StaticConfig
Parse(const yaml_config::YamlConfig& value, formats::parse::To<LinearController::StaticConfig>) {
    LinearController::StaticConfig config;
    config.fake_mode = value["fake-mode"].As<bool>(false);
    config.enabled = value["enabled"].As<bool>(true);
    return config;
}

}  // namespace congestion_control::v2

USERVER_NAMESPACE_END
