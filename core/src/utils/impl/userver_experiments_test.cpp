#include <userver/utils/impl/userver_experiments.hpp>

#include <gtest/gtest.h>

#include <userver/utest/assert_macros.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

utils::impl::UserverExperiment foo_experiment{"foo"};
utils::impl::UserverExperiment bar_experiment{"bar"};

}  // namespace

TEST(UserverExperiments, DefaultDisabled) {
  EXPECT_FALSE(foo_experiment.IsEnabled());
}

TEST(UserverExperiments, SetScope) {
  {
    utils::impl::UserverExperimentsScope exp_scope;
    exp_scope.Set(foo_experiment, true);
    EXPECT_TRUE(foo_experiment.IsEnabled());
    EXPECT_FALSE(bar_experiment.IsEnabled());
  }
  EXPECT_FALSE(foo_experiment.IsEnabled());
  EXPECT_FALSE(bar_experiment.IsEnabled());
}

TEST(UserverExperiments, NestedScope) {
  {
    utils::impl::UserverExperimentsScope exp_scope;
    exp_scope.Set(foo_experiment, true);
    EXPECT_TRUE(foo_experiment.IsEnabled());

    {
      utils::impl::UserverExperimentsScope exp_scope_nested;
      exp_scope_nested.Set(foo_experiment, false);
      EXPECT_FALSE(foo_experiment.IsEnabled());
    }

    EXPECT_TRUE(foo_experiment.IsEnabled());
  }
  EXPECT_FALSE(foo_experiment.IsEnabled());
}

TEST(UserverExperiments, EnableOnlyScope) {
  {
    utils::impl::UserverExperimentsScope exp_scope;
    exp_scope.EnableOnly({"foo"});
    EXPECT_TRUE(foo_experiment.IsEnabled());
    EXPECT_FALSE(bar_experiment.IsEnabled());
  }
  EXPECT_FALSE(foo_experiment.IsEnabled());
  EXPECT_FALSE(bar_experiment.IsEnabled());
}

TEST(UserverExperiments, EnableOnlyInvalid) {
  utils::impl::UserverExperimentsScope exp_scope;
  UASSERT_THROW_MSG(exp_scope.EnableOnly({"foo", "invalid"}),
                    utils::impl::InvalidUserverExperiments,
                    "Unknown userver experiment 'invalid'");
}

TEST(UserverExperiments, EnableOnlyInvalidResets) {
  {
    utils::impl::UserverExperimentsScope exp_scope;
    UASSERT_THROW(exp_scope.EnableOnly({"foo", "invalid"}),
                  utils::impl::InvalidUserverExperiments);
  }
  EXPECT_FALSE(foo_experiment.IsEnabled());
  EXPECT_FALSE(bar_experiment.IsEnabled());
}

USERVER_NAMESPACE_END
