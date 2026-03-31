#include <engine/deadlock_detector.hpp>

#include <unordered_map>
#include <utility>
#include <vector>

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <boost/config.hpp>
#include <boost/stacktrace.hpp>

#include <engine/task/task_context.hpp>
#include <engine/task/task_processor.hpp>
#include <engine/task/task_processor_pools.hpp>

#include <userver/concurrent/variable.hpp>
#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::deadlock_detector {

namespace {

class CycleDetector final {
public:
    using Actor = engine::impl::deadlock_detector::Actor;
    using Vertex = const Actor*;

    explicit CycleDetector(const std::unordered_map<const Actor*, std::vector<const Actor*>>& edges)
        : edges_(edges)
    {}

    std::optional<std::vector<Vertex>> FindCycle(Vertex start) {
        HandleVertex(start, nullptr);

        try {
            DoFindCycle();
        } catch (CycleException& exc) {
            return std::move(exc.cycle);
        }

        return {};
    }

private:
    struct DfsStackItem {
        Vertex v{};

        // the next not-yet processed dependency of "v"
        std::vector<Vertex>::const_iterator iter;
        std::vector<Vertex>::const_iterator end;
    };

    struct CycleException final : public std::runtime_error {
        explicit CycleException(std::vector<Vertex> cycle)
            : std::runtime_error("cycle detected"),
              cycle(std::move(cycle))
        {}

        // Actors dependency cycle. Each actor (except the last) blocks the next one.
        // The last one blocks the first.
        std::vector<Vertex> cycle;
    };

    void HandleVertex(Vertex v, Vertex previous) {
        auto it_p = vertex_to_previous_vertex_.find(v);
        if (it_p != vertex_to_previous_vertex_.end()) {
            auto finish = it_p->first;

            std::vector<Vertex> cycle;
            cycle.push_back(finish);
            while (previous != finish) {
                cycle.push_back(previous);
                previous = vertex_to_previous_vertex_[previous];
            }
            throw CycleException(std::move(cycle));
        }

        auto it = edges_.find(v);
        if (it != edges_.end()) {
            dfs_processing_stack_.emplace_back(DfsStackItem{v, it->second.begin(), it->second.end()});
            vertex_to_previous_vertex_[v] = previous;
        }
    }

    void DoFindCycle() {
        while (!dfs_processing_stack_.empty()) {
            auto& frame = dfs_processing_stack_.back();
            while (frame.iter != frame.end && processed_.find(*frame.iter) != processed_.end()) {
                ++frame.iter;
            }
            if (frame.iter == frame.end) {
                vertex_to_previous_vertex_.erase(frame.v);
                processed_.emplace(frame.v);
                dfs_processing_stack_.pop_back();
                continue;
            }

            auto next = *frame.iter++;
            HandleVertex(next, frame.v);
        }
    }

    const std::unordered_map<const Actor*, std::vector<const Actor*>>& edges_;

    std::vector<DfsStackItem> dfs_processing_stack_;

    // Allows both visited vertex tracking and backward path reconstruction
    std::unordered_map<Vertex, Vertex> vertex_to_previous_vertex_;

    std::unordered_set<Vertex> processed_;
};

}  // namespace

struct StateBase::Impl {
    std::atomic<bool> enabled{false};
    std::atomic<bool> collect_stacktrace{false};
    concurrent::Variable<std::unordered_map<const Actor*, boost::stacktrace::stacktrace>, std::mutex> backtraces;

    // "first" depends on "second"
    concurrent::Variable<std::unordered_map<const Actor*, std::vector<const Actor*>>, std::mutex> active_dependencies;
};

StateBase::StateBase(DeadlockDetector dd) {
    impl_->enabled = dd != DeadlockDetector::kOff;
    impl_->collect_stacktrace = dd == DeadlockDetector::kOn;
    LOG_INFO()
        << "Deadlock detector is " << (impl_->enabled ? "enabled" : "disabled") << ", stacktraces collection is "
        << (impl_->collect_stacktrace ? "enabled" : "disabled");
}

StateBase::~StateBase() = default;

void StateBase::AddDependency(const Actor& from, const Actor& to, bool allow_repeated_deps) {
    UASSERT(impl_->enabled);

    const auto* current = current_task::GetCurrentTaskContextUnchecked();
    if (current != nullptr && current == &from && impl_->collect_stacktrace) {
        boost::stacktrace::stacktrace trace{};
        auto bt = impl_->backtraces.Lock();
        bt->emplace(current, std::move(trace));
    }

    auto edges = impl_->active_dependencies.Lock();
    auto& dependencies = (*edges)[&from];
    UASSERT_MSG(
        allow_repeated_deps || std::find(dependencies.begin(), dependencies.end(), &to) == dependencies.end(),
        fmt::format("Adding already existing unique dependency {} -> {}", ToAssertString(from), ToAssertString(to))
    );
    dependencies.emplace_back(&to);
    CycleDetector cd(*edges);
    auto cycle = cd.FindCycle(&to);
    if (BOOST_LIKELY(!cycle.has_value())) {
        return;
    }
    if (impl_->collect_stacktrace) {
        auto bt = impl_->backtraces.Lock();
        for (const auto& actor : *cycle) {
            auto it = bt->find(actor);
            if (it == bt->end()) {
                continue;
            }
            LOG_CRITICAL() << "Deadlocked task " << ToAssertString(*actor) << boost::stacktrace::to_string(it->second);
        }
    } else {
        LOG_CRITICAL()
            << "A deadlock has been identified, but stacktrace collection is currently disabled. To enable "
               "stacktrace collection, set the `coro_pool.deadlock_detector` option in the "
               "`components::ManagerControllerComponent` static configuration to `enabled`.";
    }

    OnCycleFound(*cycle);
}

void StateBase::RemoveDependency(const Actor& from, const Actor& to) noexcept {
    UASSERT(impl_->enabled);

    auto edges = impl_->active_dependencies.Lock();
    auto& v = (*edges)[&from];

    auto it = std::find(v.begin(), v.end(), &to);
    if (it == v.end()) {
        utils::AbortWithStacktrace(fmt::format(
            "Trying to remove dependency that does not exist! {} => {}",
            ToAssertString(from),
            ToAssertString(to)
        ));
    }
    if (v.size() == 1) {
        edges->erase(&from);
        return;
    }
    std::swap(*it, v.back());
    v.pop_back();
}

void StateBase::OnResourceAcquire(const Actor& owner, const Actor& resource) {
    if (!impl_->enabled) {
        return;
    }
    // Resource release is dependent on the owner
    AddDependency(resource, owner, false);
}

void StateBase::OnResourceAcquire(const Actor& resource) {
    if (!impl_->enabled) {
        return;
    }
    // Resource release is dependent on the owner
    AddDependency(resource, current_task::GetCurrentTaskContext(), false);
}

void StateBase::OnReentrantResourceAcquire(const Actor& owner, const Actor& resource) {
    if (!impl_->enabled) {
        return;
    }
    // Resource release is dependent on the owner
    AddDependency(resource, owner, true);
}

void StateBase::OnReentrantResourceAcquire(const Actor& resource) {
    if (!impl_->enabled) {
        return;
    }
    // Resource release is dependent on the owner
    AddDependency(resource, current_task::GetCurrentTaskContext(), true);
}

void StateBase::OnResourceRelease(const Actor& owner, const Actor& resource) noexcept {
    if (!impl_->enabled) {
        return;
    }
    RemoveDependency(resource, owner);
}

void StateBase::OnResourceRelease(const Actor& resource) noexcept {
    if (!impl_->enabled) {
        return;
    }
    RemoveDependency(resource, current_task::GetCurrentTaskContext());
}

void StateBase::OnWaitForResourceStart(const Actor& waiting, const Actor& resource) {
    if (!impl_->enabled) {
        return;
    }
    AddDependency(waiting, resource, false);
}

void StateBase::OnWaitForResourceStart(const Actor& resource) {
    if (!impl_->enabled) {
        return;
    }
    AddDependency(current_task::GetCurrentTaskContext(), resource, false);
}

void StateBase::OnWaitForResourceFinish(const Actor& waiting, const Actor& resource) noexcept {
    if (!impl_->enabled) {
        return;
    }
    RemoveDependency(waiting, resource);
}

void StateBase::OnWaitForResourceFinish(const Actor& resource) noexcept {
    if (!impl_->enabled) {
        return;
    }
    RemoveDependency(current_task::GetCurrentTaskContext(), resource);
}

void StateBase::OnActorDestroy(const Actor& actor) {
    if (!impl_->enabled) {
        return;
    }
    auto edges = impl_->active_dependencies.Lock();
    auto it = edges->find(&actor);
    if (it != edges->end() && !it->second.empty()) {
        utils::AbortWithStacktrace(fmt::format(
            "Trying to destroy {} while still waiting for {}!",
            ToAssertString(actor),
            ToAssertString(*it->second.front())
        ));
    }
}

void State::OnCycleFound(const std::vector<const Actor*>& cycle) {
    utils::AbortWithStacktrace(fmt::format(
        "Found cycle: {}",
        fmt::join(cycle | boost::adaptors::transformed([](const Actor* v) { return ToAssertString(*v); }), " => ")
    ));
}

// The state is global to the process
// because a deadlock may spread across multiple TaskProcessors
State& GetState() {
    auto& pool = current_task::GetTaskProcessor().GetTaskProcessorPoolsRef();
    return pool.GetDeadlockDetectorState();
}

WaitScope::WaitScope(const engine::impl::deadlock_detector::Actor& resource)
    : resource_(resource)
{
    auto& dd_state = GetState();
    dd_state.OnWaitForResourceStart(resource_);
}

WaitScope::~WaitScope() {
    auto& dd_state = GetState();
    dd_state.OnWaitForResourceFinish(resource_);
}

}  // namespace engine::deadlock_detector

USERVER_NAMESPACE_END
