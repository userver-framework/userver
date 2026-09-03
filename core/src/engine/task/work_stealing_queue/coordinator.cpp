#include <engine/task/work_stealing_queue/coordinator.hpp>

#include <engine/task/work_stealing_queue/worker.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::fast {

namespace {

// ┌─────────────────────────────┬─────────────────────────────┐
// │  bits 63..32  (idle)        │  bits 31..0   (spinning)    │
// └─────────────────────────────┴─────────────────────────────┘
constexpr std::uint64_t kIdleWorker = static_cast<std::uint64_t>(1) << 32;

std::uint64_t AddSpinner(std::uint64_t state) noexcept { return state + 1; }

}  // namespace

Coordinator::Coordinator(const std::size_t threads) noexcept : threads_(threads) {}

Coordinator::StateView Coordinator::AsStateView(const std::uint64_t state) noexcept {
    return {static_cast<std::uint32_t>(state >> 32), static_cast<std::uint32_t>(state & (kIdleWorker - 1))};
}

bool Coordinator::StartSpinning() noexcept {
    std::uint64_t curr = state_.load();

    while (true) {
        const auto view = AsStateView(curr);

        if ((view.spinning + 1) * 2 > threads_) {
            return false;
        }

        const std::uint64_t next = AddSpinner(curr);

        if (state_.compare_exchange_weak(curr, next)) {
            return true;
        }
    }
}

bool Coordinator::StopSpinning() noexcept {
    const std::uint64_t old = state_.fetch_sub(1);
    const auto view = AsStateView(old);
    return view.spinning == 1;
}

bool Coordinator::ShouldWakeWorker() const noexcept {
    const auto view = AsStateView(state_.load());
    return (view.idle > 0) && (view.spinning == 0);
}

void Coordinator::IncrIdleCount() noexcept { state_.fetch_add(kIdleWorker); }

void Coordinator::DecrIdleCount() noexcept { state_.fetch_sub(kIdleWorker); }

void Coordinator::StepDownAsActiveWorker(Worker& worker) noexcept {
    const std::lock_guard lock{mutex_};
    if (worker.is_linked()) {
        return;
    }
    idle_workers_.push_back(worker);
    IncrIdleCount();
}

void Coordinator::BecomeActiveWorker(Worker& worker) noexcept {
    const std::lock_guard lock{mutex_};
    if (worker.is_linked()) {
        idle_workers_.erase(idle_workers_.iterator_to(worker));
        DecrIdleCount();
    }
}

void Coordinator::WakeWorker() noexcept {
    Worker* worker = nullptr;
    {
        const std::lock_guard lock{mutex_};

        if (!idle_workers_.empty()) {
            worker = &idle_workers_.back();
            idle_workers_.pop_back();
            DecrIdleCount();
        }
    }

    if (worker) {
        worker->Wake();
    }
}

void Coordinator::NotifyNewTask() noexcept {
    if (ShouldWakeWorker()) {
        WakeWorker();
    }
}

void Coordinator::RequestStop() noexcept {
    stop_requested_.store(true);
    WakeAll();
}

bool Coordinator::IsStopRequested() const noexcept { return stop_requested_.load(); }

void Coordinator::WakeAll() noexcept {
    while (true) {
        Worker* worker = nullptr;
        {
            const std::lock_guard lock{mutex_};

            if (idle_workers_.empty()) {
                return;
            }

            worker = &idle_workers_.back();
            idle_workers_.pop_back();
            DecrIdleCount();
        }

        UASSERT(worker);
        worker->Wake();
    }
}

}  // namespace engine::fast

USERVER_NAMESPACE_END
