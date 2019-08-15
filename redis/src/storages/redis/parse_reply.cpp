#include <storages/redis/parse_reply.hpp>

#include <storages/redis/reply.hpp>

namespace storages {
namespace redis {
namespace {

const std::string kOk{"OK"};
const std::string kPong{"PONG"};

std::string ExtractStringElem(ReplyData& array_data, size_t elem_idx,
                              const std::string& request_description) {
  auto& array = array_data.GetArray();
  auto& elem = array.at(elem_idx);
  if (!elem.IsString()) {
    throw ::redis::ParseReplyException(
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
    throw ::redis::ParseReplyException("Can't parse response to '" +
                                       request_description +
                                       "' request: " + ex.what());
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

std::vector<boost::optional<std::string>> ParseReplyDataArray(
    ReplyData&& array_data, const std::string& request_description,
    To<std::vector<boost::optional<std::string>>>) {
  const auto& array = array_data.GetArray();
  std::vector<boost::optional<std::string>> result;
  result.reserve(array.size());

  for (size_t elem_idx = 0; elem_idx < array.size(); ++elem_idx) {
    const auto& elem = array[elem_idx];
    if (elem.IsNil()) {
      result.emplace_back(boost::none);
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
    double score;
    try {
      score = std::stod(score_elem);
    } catch (const std::exception& ex) {
      throw ::redis::ParseReplyException(
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
    throw ::redis::ParseReplyException(
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

HsetReply Parse(ReplyData&& reply_data, const std::string& request_description,
                To<HsetReply>) {
  reply_data.ExpectInt(request_description);
  auto result = reply_data.GetInt();
  if (result < 0 || result > 1)
    throw ::redis::ParseReplyException("Unexpected reply to '" +
                                       request_description +
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
      throw ::redis::ParseReplyException("Unexpected reply to '" +
                                         request_description +
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
    throw ::redis::ParseReplyException("Unexpected redis reply to '" +
                                       request_description + "' request. " +
                                       ex.what());
  }
}

void Parse(ReplyData&& reply_data, const std::string& request_description,
           To<StatusOk, void>) {
  reply_data.ExpectStatusEqualTo(kOk, request_description);
}

bool Parse(ReplyData&& reply_data, const std::string& request_description,
           To<boost::optional<StatusOk>, bool>) {
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
  return reply_data;
}

}  // namespace redis
}  // namespace storages
