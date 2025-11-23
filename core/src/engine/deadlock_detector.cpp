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

        std::vector<Vertex> cycle;
    };

    void HandleVertex(Vertex v, Vertex parent) {
        auto it_p = parents_.find(v);
        if (it_p != parents_.end()) {
            auto finish = it_p->first;

            std::vector<Vertex> cycle;
            cycle.push_back(finish);
            while (parent != finish) {
                cycle.push_back(parent);
                parent = parents_[parent];
            }
            throw CycleException(std::move(cycle));
        }

        auto it = edges_.find(v);
        if (it != edges_.end()) {
            dfs_processing_stack_.emplace_back(DfsStackItem{v, it->second.begin(), it->second.end()});
            parents_[v] = parent;
        }
    }

    void DoFindCycle() {
        while (!dfs_processing_stack_.empty()) {
            auto& frame = dfs_processing_stack_.back();
            if (frame.iter == frame.end) {
                dfs_processing_stack_.pop_back();
                continue;
            }

            auto next = *frame.iter++;
            HandleVertex(next, frame.v);
        }
    }

    const std::unordered_map<const Actor*, std::list<const Actor*>>& edges_;

    std::vector<DfsStackItem> dfs_processing_stack_;

    // child -> parent
    std::unordered_map<Vertex, Vertex> parents_;
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

void StateBase::HookBeforeAddDependency(const Actor& subject, const Actor& object) {
    if (!impl_->enabled) {
        return;
    }

    const auto* current = current_task::GetCurrentTaskContextUnchecked();
    if (current && current == &subject && impl_->collect_stacktrace) {
        auto bt = impl_->backtraces.Lock();
        bt->emplace(current, boost::stacktrace::stacktrace());
    }

    auto edges = impl_->active_dependencies.Lock();
    auto& subject_dependencies = (*edges)[&subject];
    UASSERT_MSG(
        std::find(subject_dependencies.begin(), subject_dependencies.end(), &object) == subject_dependencies.end(),
        fmt::format("Adding already existing dependency {} -> {}", ToAssertString(subject), ToAssertString(object))
    );
    subject_dependencies.emplace_back(&object);

    CycleDetector cd(*edges);
    auto cycle = cd.FindCycle(&object);
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

void StateBase::HookBeforeRemoveDependency(const Actor& subject, const Actor& object) noexcept {
    if (!impl_->enabled) {
        return;
    }

    auto edges = impl_->active_dependencies.Lock();
    auto& v = (*edges)[&subject];

    auto it = std::find(v.begin(), v.end(), &object);
    if (it == v.end()) {
        utils::AbortWithStacktrace(fmt::format(
            "Trying to stop waiting while not waiting! {} => {}",
            ToAssertString(subject),
            ToAssertString(object)
        ));
    }
    v.erase(it);
    if (v.empty()) {
        edges->erase(&subject);
    }
}

void StateBase::HookActorDestroy(const Actor& object) {
    auto edges = impl_->active_dependencies.Lock();
    auto it = edges->find(&object);
    if (it != edges->end() && !it->second.empty()) {
        utils::AbortWithStacktrace(fmt::format(
            "Trying to destroy {} while still waiting for {}!",
            ToAssertString(object),
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

WaitScope::WaitScope(const Actor& a)
    : actor_(a)
{
    auto& dd_state = GetState();
    dd_state.HookBeforeAddDependency(current_task::GetCurrentTaskContext(), actor_);
}

WaitScope::~WaitScope() {
    auto& dd_state = GetState();
    dd_state.HookBeforeRemoveDependency(current_task::GetCurrentTaskContext(), actor_);
}

}  // namespace engine::deadlock_detector

USERVER_NAMESPACE_END
