#include <userver/storages/redis/parse_reply.hpp>

#include <userver/storages/redis/reply.hpp>
#include <userver/utils/from_string.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {
namespace {

const std::string kOk{"OK"};
const std::string kPong{"PONG"};

std::string ExtractStringElem(ReplyData& array_data, size_t elem_idx,
                              const std::string& request_description) {
  auto& array = array_data.GetArray();
  auto& elem = array.at(elem_idx);
  if (!elem.IsString()) {
    throw USERVER_NAMESPACE::redis::ParseReplyException(
        "Unexpected redis reply type to '" + request_description +
        "' request: " + "array[" + std::to_string(elem_idx) + "]: expected " +
        ReplyData::TypeToString(ReplyData::Type::kString) +
        ", got type=" + elem.GetTypeString() + " elem=" + elem.ToDebugString() +
        " array=" + array_data.ToDebugString());
  }
  return std::move(elem.GetString());
}

ReplyData::MovableKeyValues GetKeyValues(
    ReplyData& array_data, const std::string& request_description) {
  try {
    return array_data.GetMovableKeyValues();
  } catch (const std::exception& ex) {
    throw USERVER_NAMESPACE::redis::ParseReplyException(
        "Can't parse response to '" + request_description +
        "' request: " + ex.what());
  }
}

Point ParsePointArray(const redis::ReplyData& elem,
                      const std::string& request_description) {
  const auto& array = elem.GetArray();
  size_t size = array.size();
  if (size != 2) {
    throw USERVER_NAMESPACE::redis::ParseReplyException(
        "Unexpected reply to '" + request_description +
        "'. Expected 2 elements in array, got " + std::to_string(size));
  }
  if (!array[0].IsString() || !array[1].IsString()) {
    throw USERVER_NAMESPACE::redis::ParseReplyException(
        "Unexpected reply to '" + request_description +
        "'. Expected 2 elements of type: [" +
        ReplyData::TypeToString(ReplyData::Type::kString) + "," +
        ReplyData::TypeToString(ReplyData::Type::kString) +
        "], got: " + elem.ToDebugString());
  }
  try {
    return {utils::FromString<double>(array[0].GetString()),
            utils::FromString<double>(array[1].GetString())};
  } catch (const std::exception& exc) {
    throw USERVER_NAMESPACE::redis::ParseReplyException(
        "Unexpected reply to '" + request_description +
        "'. Parse sub_elements " + elem.ToDebugString() +
        " made error: " + exc.what());
  }
}

}  // namespace

namespace impl {

ReplyData&& ExtractData(ReplyPtr& reply) { return std::move(reply->data); }

bool IsNil(const ReplyData& reply_data) { return reply_data.IsNil(); }

void ExpectIsOk(const ReplyPtr& reply, const std::string& request_description) {
  reply->ExpectIsOk(request_description);
}

void ExpectArray(const ReplyData& reply_data,
                 const std::string& request_description) {
  reply_data.ExpectArray(request_description);
}

const std::string& RequestDescription(const ReplyPtr& reply,
                                      const std::string& request_description) {
  return reply->GetRequestDescription(request_description);
}

}  // namespace impl

std::vector<std::string> ParseReplyDataArray(
    ReplyData&& array_data, const std::string& request_description,
    To<std::vector<std::string>>) {
  const auto& array = array_data.GetArray();
  std::vector<std::string> result;
  result.reserve(array.size());

  for (size_t elem_idx = 0; elem_idx < array.size(); ++elem_idx) {
    result.emplace_back(
        ExtractStringElem(array_data, elem_idx, request_description));
  }
  return result;
}

std::vector<std::optional<std::string>> ParseReplyDataArray(
    ReplyData&& array_data, const std::string& request_description,
    To<std::vector<std::optional<std::string>>>) {
  const auto& array = array_data.GetArray();
  std::vector<std::optional<std::string>> result;
  result.reserve(array.size());

  for (size_t elem_idx = 0; elem_idx < array.size(); ++elem_idx) {
    const auto& elem = array[elem_idx];
    if (elem.IsNil()) {
      result.emplace_back(std::nullopt);
      continue;
    }
    result.emplace_back(
        ExtractStringElem(array_data, elem_idx, request_description));
  }
  return result;
}

std::vector<std::pair<std::string, std::string>> ParseReplyDataArray(
    ReplyData&& array_data, const std::string& request_description,
    To<std::vector<std::pair<std::string, std::string>>>) {
  auto key_values = GetKeyValues(array_data, request_description);

  std::vector<std::pair<std::string, std::string>> result;

  result.reserve(key_values.size());

  for (auto elem : key_values) {
    result.emplace_back(std::move(elem.Key()), std::move(elem.Value()));
  }
  return result;
}

std::vector<MemberScore> ParseReplyDataArray(
    ReplyData&& array_data, const std::string& request_description,
    To<std::vector<MemberScore>>) {
  auto key_values = GetKeyValues(array_data, request_description);

  std::vector<MemberScore> result;

  result.reserve(key_values.size());

  for (auto elem : key_values) {
    auto& member_elem = elem.Key();
    const auto& score_elem = elem.Value();
    double score = NAN;
    try {
      score = std::stod(score_elem);
    } catch (const std::exception& ex) {
      throw USERVER_NAMESPACE::redis::ParseReplyException(
          std::string("Can't parse response to '")
              .append(request_description)
              .append("' request: can't parse score from '")
              .append(score_elem)
              .append("' array=")
              .append(array_data.ToDebugString())
              .append(": ")
              .append(ex.what()));
    }

    result.emplace_back(std::move(member_elem), score);
  }
  return result;
}

std::vector<GeoPoint> ParseReplyDataArray(
    ReplyData&& array_data,
    [[maybe_unused]] const std::string& request_description,
    To<std::vector<GeoPoint>>) {
  std::vector<GeoPoint> result;

  for (auto& elem : array_data.GetArray()) {
    GeoPoint geo_point;
    if (elem.IsString()) {
      geo_point.member = std::move(elem.GetString());
    } else if (elem.IsArray()) {
      auto additional_infos = elem.GetArray();
      if (additional_infos.empty()) {
        throw USERVER_NAMESPACE::redis::ParseReplyException(
            "Can't parse value from reply to '" + request_description +
            ", additional_info item is empty array");
      }
      geo_point.member = additional_infos[0].GetString();

      for (size_t i = 1; i < additional_infos.size(); ++i) {
        const auto& sub_elem = additional_infos[i];
        if (sub_elem.IsInt()) {
          geo_point.hash = sub_elem.GetInt();
        } else if (sub_elem.IsString()) {
          try {
            geo_point.dist = utils::FromString<double>(sub_elem.GetString());
          } catch (const std::exception& exc) {
            throw USERVER_NAMESPACE::redis::ParseReplyException(
                "Unexpected reply to '" + request_description +
                "'. Parse sub_elem '" + sub_elem.ToDebugString() +
                "' made error: " + exc.what());
          }
        } else if (sub_elem.IsArray()) {
          geo_point.point = ParsePointArray(sub_elem, request_description);
        } else {
          throw USERVER_NAMESPACE::redis::ParseReplyException(
              "Can't parse value from reply to '" + request_description +
              ", expected subarray types: [" +
              ReplyData::TypeToString(ReplyData::Type::kString) + "," +
              ReplyData::TypeToString(ReplyData::Type::kInteger) + "," +
              ReplyData::TypeToString(ReplyData::Type::kArray) + "," +
              "], got type: " + sub_elem.GetTypeString() +
              " elem=" + sub_elem.ToDebugString());
        }
      }
    } else {
      throw USERVER_NAMESPACE::redis::ParseReplyException(
          "Can't parse value from reply to '" + request_description +
          "', expected one of [" +
          ReplyData::TypeToString(ReplyData::Type::kString) + ", " +
          ReplyData::TypeToString(ReplyData::Type::kArray) + "], got type: " +
          elem.GetTypeString() + " elem: " + elem.ToDebugString());
    }
    result.push_back(std::move(geo_point));
  }
  return result;
}

std::string Parse(ReplyData&& reply_data,
                  const std::string& request_description, To<std::string>) {
  reply_data.ExpectString(request_description);
  return std::move(reply_data.GetString());
}

double Parse(ReplyData&& reply_data, const std::string& request_description,
             To<double>) {
  reply_data.ExpectString(request_description);
  try {
    return std::stod(reply_data.GetString());
  } catch (const std::exception& ex) {
    throw USERVER_NAMESPACE::redis::ParseReplyException(
        "Can't parse value from reply to '" + request_description +
        "' request (" + reply_data.ToDebugString() + "): " + ex.what());
  }
}

size_t Parse(ReplyData&& reply_data, const std::string& request_description,
             To<size_t>) {
  reply_data.ExpectInt(request_description);
  return reply_data.GetInt();
}

bool Parse(ReplyData&& reply_data, const std::string& request_description,
           To<size_t, bool>) {
  reply_data.ExpectInt(request_description);
  return !!reply_data.GetInt();
}

int64_t Parse(ReplyData&& reply_data, const std::string& request_description,
              To<int64_t>) {
  reply_data.ExpectInt(request_description);
  return reply_data.GetInt();
}

std::chrono::system_clock::time_point Parse(
    ReplyData&& reply_data, const std::string& request_description,
    To<std::chrono::system_clock::time_point>) {
  reply_data.ExpectArray(request_description);
  auto&& result = reply_data.GetArray();
  if (result.size() != 2) {
    throw USERVER_NAMESPACE::redis::ParseReplyException(
        "Unexpected reply to '" + request_description +
        "'. Expected 2 elements in array, got " +
        std::to_string(result.size()));
  }
  for (auto&& i : result) {
    i.ExpectString(request_description);
  }
  return std::chrono::system_clock::time_point(
      std::chrono::seconds(std::stoi(result[0].GetString())) +
      std::chrono::microseconds(std::stoi(result[1].GetString())));
}

HsetReply Parse(ReplyData&& reply_data, const std::string& request_description,
                To<HsetReply>) {
  reply_data.ExpectInt(request_description);
  auto result = reply_data.GetInt();
  if (result < 0 || result > 1)
    throw USERVER_NAMESPACE::redis::ParseReplyException(
        "Unexpected reply to '" + request_description +
        "' request: " + std::to_string(result));
  return result ? HsetReply::kCreated : HsetReply::kUpdated;
}

PersistReply Parse(ReplyData&& reply_data,
                   const std::string& request_description, To<PersistReply>) {
  reply_data.ExpectInt(request_description);
  auto value = reply_data.GetInt();
  switch (value) {
    case 0:
      return PersistReply::kKeyOrTimeoutNotFound;
    case 1:
      return PersistReply::kTimeoutRemoved;
    default:
      throw USERVER_NAMESPACE::redis::ParseReplyException(
          "Unexpected reply to '" + request_description +
          "' request: " + std::to_string(value));
  }
}

KeyType Parse(ReplyData&& reply_data, const std::string& request_description,
              To<KeyType>) {
  reply_data.ExpectStatus(request_description);
  const auto& status = reply_data.GetStatus();
  try {
    return ParseKeyType(status);
  } catch (const std::exception& ex) {
    throw USERVER_NAMESPACE::redis::ParseReplyException(
        "Unexpected redis reply to '" + request_description + "' request. " +
        ex.what());
  }
}

void Parse(ReplyData&& reply_data, const std::string& request_description,
           To<StatusOk, void>) {
  reply_data.ExpectStatusEqualTo(kOk, request_description);
}

bool Parse(ReplyData&& reply_data, const std::string& request_description,
           To<std::optional<StatusOk>, bool>) {
  if (reply_data.IsNil()) return false;
  reply_data.ExpectStatusEqualTo(kOk, request_description);
  return true;
}

void Parse(ReplyData&& reply_data, const std::string& request_description,
           To<StatusPong, void>) {
  reply_data.ExpectStatusEqualTo(kPong, request_description);
}

SetReply Parse(ReplyData&& reply_data, const std::string& request_description,
               To<SetReply>) {
  if (reply_data.IsNil()) return SetReply::kNotSet;
  reply_data.ExpectStatusEqualTo(kOk, request_description);
  return SetReply::kSet;
}

std::unordered_set<std::string> Parse(ReplyData&& reply_data,
                                      const std::string& request_description,
                                      To<std::unordered_set<std::string>>) {
  reply_data.ExpectArray(request_description);

  const auto& array = reply_data.GetArray();
  std::unordered_set<std::string> result;
  result.reserve(array.size());

  for (size_t elem_idx = 0; elem_idx < array.size(); ++elem_idx) {
    result.emplace(
        ExtractStringElem(reply_data, elem_idx, request_description));
  }
  return result;
}

std::unordered_map<std::string, std::string> Parse(
    ReplyData&& reply_data, const std::string& request_description,
    To<std::unordered_map<std::string, std::string>>) {
  reply_data.ExpectArray(request_description);

  auto key_values = GetKeyValues(reply_data, request_description);

  std::unordered_map<std::string, std::string> result;

  result.reserve(key_values.size());

  for (auto elem : key_values) {
    result[std::move(elem.Key())] = std::move(elem.Value());
  }
  return result;
}

ReplyData Parse(ReplyData&& reply_data, const std::string&, To<ReplyData>) {
  return std::move(reply_data);
}

}  // namespace storages::redis

USERVER_NAMESPACE_END
