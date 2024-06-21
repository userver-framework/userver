#include <userver/cache/statistics_mock.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache {

UpdateStatisticsScopeMock::UpdateStatisticsScopeMock(UpdateType type)
    : scope_(stats_, type) {}

UpdateStatisticsScope& UpdateStatisticsScopeMock::GetScope() { return scope_; }

}  // namespace cache

USERVER_NAMESPACE_END
