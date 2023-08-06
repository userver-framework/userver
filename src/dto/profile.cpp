#include "profile.hpp"


namespace realmedium::dto {

userver::formats::json::Value Serialize(
    const realmedium::dto::Profile& dto,
    userver::formats::serialize::To<userver::formats::json::Value>) {
  return {};
}

}  // realmedium::dto