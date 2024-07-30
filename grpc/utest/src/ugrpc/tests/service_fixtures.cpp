#include <userver/ugrpc/tests/service_fixtures.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::tests {

utils::statistics::Snapshot ServiceFixtureBase::GetStatistics(
    std::string prefix, std::vector<utils::statistics::Label> require_labels) {
  return utils::statistics::Snapshot{this->GetStatisticsStorage(),
                                     std::move(prefix),
                                     std::move(require_labels)};
}

}  // namespace ugrpc::tests

USERVER_NAMESPACE_END
