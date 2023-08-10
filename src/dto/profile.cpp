#include "profile.hpp"

namespace real_medium::dto {

userver::formats::json::Value Serialize(
    const real_medium::dto::Profile& dto,
    userver::formats::serialize::To<userver::formats::json::Value>) {
  return {};
}

}  // namespace real_medium::dto