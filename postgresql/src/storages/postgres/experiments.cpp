#include <storages/postgres/experiments.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

USERVER_NAMESPACE::utils::impl::UserverExperiment
    kOmitDescribeInExecuteExperiment{"pg-omit-describe-in-execute", true};

}

USERVER_NAMESPACE_END
