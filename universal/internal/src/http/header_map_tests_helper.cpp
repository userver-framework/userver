#include <userver/internal/http/header_map_tests_helper.hpp>

USERVER_NAMESPACE_BEGIN

namespace http::headers {

header_map::Map& TestsHelper::GetMapImpl(HeaderMap& map) { return *map.impl_; }

const header_map::Danger& TestsHelper::GetMapDanger(const HeaderMap& map) {
  return map.impl_->danger_;
}

std::size_t TestsHelper::GetMapMaxDisplacement(const HeaderMap& map) {
  return map.impl_->DebugMaxDisplacement();
}

void TestsHelper::ForceIntoRedState(HeaderMap& map) {
  GetMapImpl(map).DebugForceIntoRed();
}

}  // namespace http::headers

USERVER_NAMESPACE_END
