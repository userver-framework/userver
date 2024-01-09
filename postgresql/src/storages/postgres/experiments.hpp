#pragma once

#include <userver/utils/impl/userver_experiments.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

extern USERVER_NAMESPACE::utils::impl::UserverExperiment
    kOmitDescribeInExecuteExperiment;
inline constexpr auto kOmitDescribeExperimentVersion = 1;

inline constexpr auto kDeadlinePropagationExperimentVersion = 1;

}  // namespace storages::postgres

USERVER_NAMESPACE_END
