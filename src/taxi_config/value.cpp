#include <taxi_config/value.hpp>

#include <type_traits>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/exception/exception.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/types/value.hpp>

#include <logging/logger.hpp>
#include <redis/base.hpp>

namespace taxi_config {
namespace impl {

template <>
std::string MongoCast<std::string>(
    const storages::mongo::DocumentElement& elem) {
  return storages::mongo::ToString(elem);
}

template <>
std::unordered_set<std::string> MongoCast<std::unordered_set<std::string>>(
    const storages::mongo::DocumentElement& elem) {
  std::unordered_set<std::string> response;
  for (const auto& subitem : storages::mongo::ToArray(elem)) {
    response.emplace(storages::mongo::ToString(subitem));
  }
  return response;
}

template <>
bool MongoCast<bool>(const storages::mongo::DocumentElement& elem) {
  return storages::mongo::ToBool(elem);
}

template <>
redis::CommandControl::Strategy MongoCast<redis::CommandControl::Strategy>(
    const storages::mongo::DocumentElement& elem) {
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
    const storages::mongo::DocumentElement& elem) {
  redis::CommandControl response;

  for (const auto& e : storages::mongo::ToDocument(elem)) {
    const auto& name = e.key().to_string();
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

storages::mongo::DocumentElement DocsMap::Get(const std::string& name) const {
  const auto& mongo_it = docs_.find(name);
  if (mongo_it == docs_.end())
    throw std::runtime_error("Can't find doc for " + name);

  const auto& doc = mongo_it->second;
  auto view = doc.view();
  if (view.find("v") == view.end())
    throw std::runtime_error("Mongo config have no element 'v' for " + name);

  auto element = view["v"];
  if (!element)
    throw std::runtime_error("Mongo element is not valid for " + name);

  requested_names_.insert(name);

  return element;
}

void DocsMap::Set(std::string name, storages::mongo::DocumentValue obj) {
  auto it = docs_.find(name);
  if (it != docs_.end()) {
    it->second = std::move(obj);
  } else {
    docs_.emplace(std::move(name), std::move(obj));
  }
}

void DocsMap::Parse(const std::string& json, bool empty_ok) {
  namespace bbb = bsoncxx::builder::basic;

  auto bson = [&json] {
    try {
      return bsoncxx::from_json(json);
    } catch (const bsoncxx::exception& ex) {
      throw std::runtime_error(std::string("DocsMap::Parse failed: ") +
                               ex.what());
    }
  }();

  auto view = bson.view();
  if (!empty_ok && view.empty())
    throw std::runtime_error("DocsMap::Parse failed: BSON is empty");

  std::vector<storages::mongo::DocumentElement> elements(view.begin(),
                                                         view.end());

  if (!empty_ok && elements.empty())
    throw std::runtime_error("DocsMap::Parse failed: elements are empty");

  for (auto const& element : elements) {
    Set(element.key().to_string(),
        bbb::make_document(bbb::kvp("v", element.get_value())));
  }
}

size_t DocsMap::Size() const { return docs_.size(); }

void DocsMap::MergeFromOther(DocsMap&& other) {
  // TODO: do docs_.merge(other) after we get C++17 std lib
  for (auto& it : other.docs_) {
    std::string name = it.first;
    storages::mongo::DocumentValue value = std::move(it.second);

    auto this_it = docs_.find(name);
    if (this_it == docs_.end()) {
      docs_.emplace(std::move(name), std::move(value));
    } else {
      this_it->second = std::move(value);
    }
  }
}

std::vector<std::string> DocsMap::GetRequestedNames() const {
  return std::vector<std::string>(requested_names_.begin(),
                                  requested_names_.end());
}

}  // namespace taxi_config
