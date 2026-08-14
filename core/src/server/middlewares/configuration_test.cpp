#include <userver/utest/utest.hpp>

#include <userver/server/middlewares/builtin.hpp>
#include <userver/server/middlewares/configuration.hpp>

USERVER_NAMESPACE_BEGIN

UTEST(MiddlewaresConfiguration, MinimalPipelineIsSubsetOfDefault) {
    const auto minimal = server::middlewares::MinimalPipeline();
    const auto defaults = server::middlewares::DefaultPipeline();

    ASSERT_FALSE(minimal.empty());
    ASSERT_LT(minimal.size(), defaults.size());
}

USERVER_NAMESPACE_END
