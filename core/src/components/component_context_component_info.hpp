#pragma once

#include <atomic>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>

#include <userver/components/raw_component_base.hpp>
#include <userver/engine/condition_variable.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/utils/not_null.hpp>

USERVER_NAMESPACE_BEGIN

namespace components::impl {

enum class ComponentLifetimeStage { kNull, kCreated, kRunning, kReadyForClearing };

class StageSwitchingCancelledException : public std::runtime_error {
public:
    explicit StageSwitchingCancelledException(const std::string& message);
};

class ComponentInfo;
using ComponentInfoRef = utils::NotNull<ComponentInfo*>;
using ConstComponentInfoRef = utils::NotNull<const ComponentInfo*>;

class ComponentInfo final {
public:
    explicit ComponentInfo(std::string name);

    std::string_view GetName() const { return name_; }

    void SetComponent(std::unique_ptr<RawComponentBase>&& component);
    void ClearComponent();
    RawComponentBase* GetComponent() const;
    RawComponentBase* WaitAndGetComponent() const;

    void AddItDependsOn(ComponentInfo& component);
    void AddDependsOnIt(ComponentInfo& component);

    bool CheckItDependsOn(ComponentInfo& component) const;
    bool CheckDependsOnIt(ComponentInfo& component) const;

    template <typename Func>
    void ForEachItDependsOn(const Func& func) const {
        std::unique_lock lock{mutex_};
        if (stage_ == ComponentLifetimeStage::kNull) {
            auto it_depends_on = it_depends_on_;
            lock.unlock();

            for (const auto& component : it_depends_on) {
                func(*component);
            }
        } else {
            lock.unlock();

            // Only readers remain, could read without copying
            for (const auto& component : it_depends_on_) {
                func(*component);
            }
        }
    }

    template <typename Func>
    void ForEachDependsOnIt(const Func& func) const {
        std::unique_lock lock{mutex_};
        if (stage_ == ComponentLifetimeStage::kNull || stage_ == ComponentLifetimeStage::kCreated) {
            auto depends_on_it = depends_on_it_;
            lock.unlock();

            for (const auto& component : depends_on_it) {
                func(*component);
            }
        } else {
            lock.unlock();

            // Only readers remain, could read without copying
            for (const auto& component : depends_on_it_) {
                func(*component);
            }
        }
    }

    void SetStageSwitchingCancelled(bool cancelled);

    void OnLoadingCancelled();
    void OnAllComponentsLoaded();
    void OnAllComponentsAreStopping();

    void SetStage(ComponentLifetimeStage stage);
    ComponentLifetimeStage GetStage() const;
    void WaitStage(ComponentLifetimeStage stage, std::string method_name) const;

    std::string GetDependencies() const;

private:
    bool HasComponent() const;
    std::unique_ptr<RawComponentBase> ExtractComponent();

    const std::string name_;

    mutable engine::Mutex mutex_;
    mutable engine::ConditionVariable cv_;
    std::unique_ptr<RawComponentBase> component_;
    std::set<ComponentInfoRef> it_depends_on_;
    std::set<ComponentInfoRef> depends_on_it_;
    ComponentLifetimeStage stage_ = ComponentLifetimeStage::kNull;
    bool stage_switching_cancelled_{false};
    std::atomic<bool> on_loading_cancelled_called_{false};
};

std::string JoinNamesFromInfo(const std::vector<ConstComponentInfoRef>& container, std::string_view separator);

std::string JoinNamesFromInfo(const std::set<ComponentInfoRef>& container, std::string_view separator);

}  // namespace components::impl

USERVER_NAMESPACE_END
