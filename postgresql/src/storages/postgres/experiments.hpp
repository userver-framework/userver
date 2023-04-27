#pragma once

#include <userver/utils/impl/userver_experiments.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

extern USERVER_NAMESPACE::utils::impl::UserverExperiment kPipelineExperiment;

extern USERVER_NAMESPACE::utils::impl::UserverExperiment
    kDeadlinePropagationExperiment;

}  // namespace storages::postgres

USERVER_NAMESPACE_END
