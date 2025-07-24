#include <userver/utils/periodic_task.hpp>

#include <random>
#include <tuple>

#include <fmt/format.h>

#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/logging/log.hpp>
#include <userver/testsuite/periodic_task_control.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/tracer.hpp>
#include <userver/utils/rand.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace {

auto TieSettings(const PeriodicTask::Settings& settings) {
    // Can't use Boost.Pfr, because Settings has custom constructors.
    const auto& [f1, f2, f3, f4, f5, f6] = settings;
    return std::tie(f1, f2, f3, f4, f5, f6);
}

}  // namespace

bool PeriodicTask::Settings::operator==(const Settings& other) const noexcept {
    return TieSettings(*this) == TieSettings(other);
}

bool PeriodicTask::Settings::operator!=(const Settings& other) const noexcept {
    return TieSettings(*this) != TieSettings(other);
}

PeriodicTask::PeriodicTask() : settings_(std::chrono::seconds(1)), suspend_state_(SuspendState::kRunning) {}

PeriodicTask::PeriodicTask(std::string name, Settings settings, Callback callback)
    : name_(std::move(name)),
      is_name_set_(true),
      callback_(std::move(callback)),
      settings_(std::move(settings)),
      suspend_state_(SuspendState::kRunning) {
    DoStart();
}

PeriodicTask::~PeriodicTask() {
    registration_holder_ = std::nullopt;
    Stop();
}

void PeriodicTask::Start(std::string name, Settings settings, Callback callback) {
    UASSERT_MSG(!name.empty(), "Periodic task must have a name");
    auto settings_ptr = settings_.StartWriteEmplace(std::move(settings));
    // Set name_ under the 'settings_' mutex.
    if (!is_name_set_) {
        name_ = std::move(name);
        is_name_set_ = true;
    } else {
        UINVARIANT(
            name_ == name, fmt::format("PeriodicTask name must not be changed on the fly, old={}, new={}", name_, name)
        );
    }
    // Stop here so that if the invariant above fails, the task is not affected.
    Stop();
    callback_ = std::move(callback);
    settings_ptr.Commit();
    DoStart();
}

void PeriodicTask::DoStart() {
    LOG_DEBUG() << "Starting PeriodicTask with name=" << GetName();
    auto settings_ptr = settings_.Read();
    auto& task_processor =
        settings_ptr->task_processor ? *settings_ptr->task_processor : engine::current_task::GetTaskProcessor();
    task_ = engine::CriticalAsyncNoSpan(task_processor, &PeriodicTask::Run, this);
}

void PeriodicTask::Stop() noexcept {
    const auto name = GetName();
    try {
        if (IsRunning()) {
            LOG_INFO() << "Stopping PeriodicTask with name=" << name;
            task_.SyncCancel();
            changed_event_.Reset();
            should_force_step_ = false;
            task_ = engine::TaskWithResult<void>();
            LOG_INFO() << "Stopped PeriodicTask with name=" << name;
        }
    } catch (std::exception& e) {
        LOG_ERROR() << "Exception while stopping PeriodicTask with name=" << name << ": " << e;
    } catch (...) {
        LOG_ERROR() << "Exception while stopping PeriodicTask with name=" << name;
    }
}

void PeriodicTask::SetSettings(Settings settings) {
    bool should_notify_task{};
    {
        auto writer = settings_.StartWrite();
        if (settings == *writer) {
            // Updating an RCU is slow, better to avoid it when possible.
            return;
        }
        settings.flags = writer->flags;
        should_notify_task = settings.period != writer->period || settings.exception_period != writer->exception_period;
        *writer = std::move(settings);
        writer.Commit();
    }

    if (should_notify_task) {
        LOG_DEBUG() << "periodic task settings have changed, signalling name=" << GetName();
        changed_event_.Send();
    }
}

void PeriodicTask::ForceStepAsync() {
    should_force_step_ = true;
    changed_event_.Send();
}

bool PeriodicTask::SynchronizeDebug(bool preserve_span) {
    if (!IsRunning()) {
        return false;
    }

    return StepDebug(preserve_span);
}

bool PeriodicTask::IsRunning() const { return task_.IsValid(); }

void PeriodicTask::Run() {
    bool skip_step = false;
    {
        auto settings = settings_.Read();
        if (!(settings->flags & Flags::kNow)) {
            skip_step = true;
        }
    }

    while (!engine::current_task::ShouldCancel()) {
        const auto before = std::chrono::steady_clock::now();
        bool no_exception = true;

        if (!std::exchange(skip_step, false)) {
            no_exception = Step();
        }

        const auto settings = settings_.Read();
        auto period = settings->period;
        const auto exception_period = settings->exception_period.value_or(period);

        if (!no_exception) period = exception_period;

        std::chrono::steady_clock::time_point start;
        if (settings->flags & Flags::kStrong) {
            start = before;
        } else {
            start = std::chrono::steady_clock::now();
        }

        while (changed_event_.WaitForEventUntil(start + MutatePeriod(period))) {
            if (should_force_step_.exchange(false)) {
                break;
            }
            // The config variable value has been changed, reload
            const auto settings = settings_.Read();
            period = settings->period;
            const auto exception_period = settings->exception_period.value_or(period);
            if (!no_exception) period = exception_period;
        }
    }
}

bool PeriodicTask::DoStep() {
    auto settings_ptr = settings_.Read();
    const auto span_log_level = settings_ptr->span_level;
    const auto name = GetName();
    tracing::Span span(std::string{name});
    span.SetLogLevel(span_log_level);
    try {
        callback_();
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR() << "Exception in PeriodicTask with name=" << name << ": " << e;
        return false;
    }
}

bool PeriodicTask::Step() {
    const std::lock_guard lock_step(step_mutex_);

    if (suspend_state_.load() == SuspendState::kSuspended) {
        LOG_INFO() << "Skipping suspended PeriodicTask with name=" << GetName();
        return true;
    }

    return DoStep();
}

bool PeriodicTask::StepDebug(bool preserve_span) {
    const std::lock_guard lock_step(step_mutex_);

    std::optional<tracing::Span> testsuite_oneshot_span;
    if (preserve_span) {
        tracing::Span span("periodic-synchronize-debug-call");
        testsuite_oneshot_span.emplace(std::move(span));
    }

    return DoStep();
}

std::chrono::milliseconds PeriodicTask::MutatePeriod(std::chrono::milliseconds period) {
    auto settings_ptr = settings_.Read();
    if (!(settings_ptr->flags & Flags::kChaotic)) return period;

    if (!mutate_period_random_) {
        mutate_period_random_.emplace(
            utils::WithDefaultRandom(std::uniform_int_distribution<std::minstd_rand::result_type>{})
        );
    }

    const auto jitter = settings_ptr->distribution;
    std::uniform_int_distribution distribution{(period - jitter).count(), (period + jitter).count()};
    const auto ms = distribution(*mutate_period_random_);
    return std::chrono::milliseconds(ms);
}

std::string_view PeriodicTask::GetName() const noexcept {
    return is_name_set_ ? std::string_view{name_} : "<name not set>";
}

void PeriodicTask::SuspendDebug() {
    // step_mutex_ waits, for a potentially long time, for Step() call completion
    const std::lock_guard lock_step(step_mutex_);
    auto prior_state = suspend_state_.exchange(SuspendState::kSuspended);
    if (prior_state != SuspendState::kSuspended) {
        LOG_DEBUG() << "Periodic task " << GetName() << " suspended";
    }
}

void PeriodicTask::ResumeDebug() {
    auto prior_state = suspend_state_.exchange(SuspendState::kRunning);
    if (prior_state != SuspendState::kRunning) {
        LOG_DEBUG() << "Periodic task " << GetName() << " resumed";
    }
}

void PeriodicTask::RegisterInTestsuite(testsuite::PeriodicTaskControl& periodic_task_control) {
    UINVARIANT(is_name_set_, "PeriodicTask::RegisterInTestsuite should be called after Start");
    registration_holder_.emplace(periodic_task_control, std::string{GetName()}, *this);
}

PeriodicTask::Settings PeriodicTask::GetCurrentSettings() const {
    auto settings_ptr = settings_.Read();
    return *settings_ptr;
}

}  // namespace utils

USERVER_NAMESPACE_END
