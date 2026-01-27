#pragma once

#include <userver/concurrent/variable.hpp>
#include <userver/rcu/rcu_map.hpp>
#include <userver/utils/retry_budget.hpp>

USERVER_NAMESPACE_BEGIN

namespace retry_budget {

class Storage final {
public:
    utils::RetryBudget& GetDestination(const std::string& destination) {
        auto it = map_.Get(destination);
        if (!it) {
            auto [it, ok] = map_.TryEmplace(std::string{destination}, default_);
            UASSERT(it);
            return *it;
        }
        return *it;
    }

    void SetPolicy(utils::RetryBudgetSettings policy) {
        auto map = map_.StartWrite();
        default_ = policy;
        for (auto& [name, dest] : *map) {
            dest->SetSettings(policy);
        }
    }

private:
    utils::RetryBudgetSettings default_;  // synchronized via map_ write mutex
    rcu::RcuMap<std::string, utils::RetryBudget> map_;
};

}  // namespace retry_budget

USERVER_NAMESPACE_END
