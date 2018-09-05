#include <taxi_config/value.hpp>

#include <type_traits>

#include <mongo/db/json.h>
#include <logging/logger.hpp>
#include <redis/base.hpp>

namespace taxi_config {
namespace impl {

template <>
std::string MongoCast<std::string>(const ::mongo::BSONElement& elem) {
  return storages::mongo::ToString(elem);
}

template <>
std::unordered_set<std::string> MongoCast<std::unordered_set<std::string>>(
    const ::mongo::BSONElement& elem) {
  std::unordered_set<std::string> response;
  for (const auto& subitem : storages::mongo::ToArray(elem)) {
    response.emplace(storages::mongo::ToString(subitem));
  }
  return response;
}

template <>
bool MongoCast<bool>(const ::mongo::BSONElement& elem) {
  return storages::mongo::ToBool(elem);
}

template <>
redis::CommandControl::Strategy MongoCast<redis::CommandControl::Strategy>(
    const ::mongo::BSONElement& elem) {
  auto strategy = MongoCast<std::string>(elem);
  if (strategy == "every_dc") {
    return redis::CommandControl::Strategy::kEveryDc;
  } else if (strategy == "default") {
    return redis::CommandControl::Strategy::kDefault;
  } else if (strategy == "local_dc_conductor") {
    return redis::CommandControl::Strategy::kLocalDcConductor;
  } else if (strategy == "nearest_server_ping") {
    return redis::CommandControl::Strategy::kNearestServerPing;
  } else {
    LOG_ERROR() << "Unknown strategy for redis::CommandControl::Strategy: "
                << strategy << ", falling back to EveryDc";
    return redis::CommandControl::Strategy::kEveryDc;
  }
}

template <>
redis::CommandControl MongoCast<redis::CommandControl>(
    const ::mongo::BSONElement& elem) {
  redis::CommandControl response;

  const auto& obj = storages::mongo::ToDocument(elem);
  for (auto i = obj.begin(); i.more(); ++i) {
    const auto& e = *i;
    const std::string& name = e.fieldName();
    if (name == "timeout_all_ms") {
      response.timeout_all = MongoCast<std::chrono::milliseconds>(e);
    } else if (name == "timeout_single_ms") {
      response.timeout_single = MongoCast<std::chrono::milliseconds>(e);
    } else if (name == "max_retries") {
      response.max_retries = MongoCast<unsigned>(e);
    } else if (name == "strategy") {
      response.strategy = MongoCast<redis::CommandControl::Strategy>(e);
    } else if (name == "best_dc_count") {
      response.best_dc_count = MongoCast<unsigned>(e);
    } else if (name == "max_ping_latency_ms") {
      response.max_ping_latency = MongoCast<std::chrono::milliseconds>(e);
    } else {
      LOG_WARNING() << "unknown key for CommandControl map: " << name;
    }
  }
  if ((response.best_dc_count > 1) &&
      (response.strategy !=
       redis::CommandControl::Strategy::kNearestServerPing)) {
    LOG_WARNING() << "CommandControl.best_dc_count = " << response.best_dc_count
                  << ", but is ignored for the current strategy ("
                  << (size_t)response.strategy << ")";
  }
  return response;
}

}  // namespace impl

mongo::BSONElement DocsMap::Get(const std::string& name) const {
  const auto& mongo_it = docs_.find(name);
  if (mongo_it == docs_.end())
    throw std::runtime_error("Can't find doc for " + name);

  const auto& doc = mongo_it->second;
  if (!doc.hasElement("v"))
    throw std::runtime_error("Mongo config have no element 'v' for " + name);

  auto element = doc["v"];
  if (!element.ok())
    throw std::runtime_error("Mongo element is not ok for " + name);

  return element;
}

void DocsMap::Set(std::string name, mongo::BSONObj obj) {
  docs_[std::move(name)] = std::move(obj);
}

void DocsMap::Parse(const std::string& json) {
  const mongo::BSONObj bson = mongo::fromjson(json);

  if (!bson.isValid())
    throw std::runtime_error("DocsMap::Parse failed: invalid BSON");

  if (bson.isEmpty())
    throw std::runtime_error("DocsMap::Parse failed: BSON is empty");

  std::vector<mongo::BSONElement> elements;
  bson.elems(elements);

  if (elements.empty())
    throw std::runtime_error("DocsMap::Parse failed: elements are empty");

  for (auto const& element : elements) {
    mongo::BSONObjBuilder builder;
    builder.appendAs(element, "v");
    Set(element.fieldName(), builder.obj());
  }
}

}  // namespace taxi_config
