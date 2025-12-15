#include <engine/deadlock_detector.hpp>

#include <list>

#include <fmt/format.h>
#include <fmt/ranges.h>
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
    using Vertex = const Actor*;

    explicit CycleDetector(const std::unordered_map<const Actor*, std::list<const Actor*>>& edges)
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
        std::list<Vertex>::const_iterator iter;
        std::list<Vertex>::const_iterator end;
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
        if (it != edges_.end() && processed_.find(v) == processed_.end()) {
            dfs_processing_stack_.emplace_back(DfsStackItem{v, it->second.begin(), it->second.end()});
            vertex_to_previous_vertex_[v] = previous;
        }
    }

    void DoFindCycle() {
        while (!dfs_processing_stack_.empty()) {
            auto& frame = dfs_processing_stack_.back();
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

    const std::unordered_map<const Actor*, std::list<const Actor*>>& edges_;

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
    concurrent::Variable<std::unordered_map<const Actor*, std::list<const Actor*>>, std::mutex> active_dependencies;
};

StateBase::StateBase(DeadlockDetector dd) {
    impl_->enabled = dd != DeadlockDetector::kOff;
    impl_->collect_stacktrace = dd == DeadlockDetector::kOn;
}

StateBase::~StateBase() = default;

void StateBase::AddDependency(const Actor& from, const Actor& to) {
    if (!impl_->enabled) {
        return;
    }

    const auto* current = current_task::GetCurrentTaskContextUnchecked();
    if (current && current == &from && impl_->collect_stacktrace) {
        auto bt = impl_->backtraces.Lock();
        bt->emplace(current, boost::stacktrace::stacktrace());
    }

    auto edges = impl_->active_dependencies.Lock();
    auto& dependencies = (*edges)[&from];
    UASSERT_MSG(
        std::find(dependencies.begin(), dependencies.end(), &to) == dependencies.end(),
        fmt::format("Adding already existing dependency {} -> {}", ToAssertString(from), ToAssertString(to))
    );
    dependencies.emplace_back(&to);
    CycleDetector cd(*edges);
    auto cycle = cd.FindCycle(&to);
    if (cycle) {
        auto bt = impl_->backtraces.Lock();
        for (const auto& actor : *cycle) {
            auto it = bt->find(actor);
            if (it != bt->end()) {
                LOG_CRITICAL()
                    << "Deadlocked task " << ToAssertString(*actor) << boost::stacktrace::to_string(it->second);
            }
        }

        OnCycleFound(*cycle);
    }
}

void StateBase::RemoveDependency(const Actor& from, const Actor& to) noexcept {
    if (!impl_->enabled) {
        return;
    }

    auto edges = impl_->active_dependencies.Lock();
    auto& v = (*edges)[&from];

    auto it = std::find(v.begin(), v.end(), &to);
    if (it == v.end()) {
        utils::AbortWithStacktrace(
            fmt::format("Trying to stop waiting while not waiting! {} => {}", ToAssertString(from), ToAssertString(to))
        );
    }
    v.erase(it);
    if (v.empty()) {
        edges->erase(&from);
    }
}

void StateBase::OnResourceAcquire(const Actor& owner, const Actor& resource) {
    // Resource release is dependent on the owner
    AddDependency(resource, owner);
}

void StateBase::OnResourceRelease(const Actor& owner, const Actor& resource) noexcept {
    RemoveDependency(resource, owner);
}

void StateBase::OnWaitForResourceStart(const Actor& waiting, const Actor& resource) {
    AddDependency(waiting, resource);
}

void StateBase::OnWaitForResourceFinish(const Actor& waiting, const Actor& resource) noexcept {
    RemoveDependency(waiting, resource);
}

void StateBase::OnActorDestroy(const Actor& actor) {
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

WaitScope::WaitScope(const Actor& resource)
    : resource_(resource)
{
    auto& dd_state = GetState();
    dd_state.OnWaitForResourceStart(current_task::GetCurrentTaskContext(), resource_);
}

WaitScope::~WaitScope() {
    auto& dd_state = GetState();
    dd_state.OnWaitForResourceFinish(current_task::GetCurrentTaskContext(), resource_);
}

}  // namespace engine::deadlock_detector

USERVER_NAMESPACE_END
