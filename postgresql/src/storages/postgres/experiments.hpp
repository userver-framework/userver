#pragma once

#include <userver/utils/impl/userver_experiments.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

extern USERVER_NAMESPACE::utils::impl::UserverExperiment kPipelineExperiment;
constexpr auto kPipelineExperimentVersion = 1;

extern USERVER_NAMESPACE::utils::impl::UserverExperiment
    kDeadlinePropagationExperiment;

}  // namespace storages::postgres

USERVER_NAMESPACE_END
