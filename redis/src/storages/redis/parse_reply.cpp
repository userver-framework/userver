#include <storages/redis/parse_reply.hpp>

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/optional.hpp>

#include <redis/reply.hpp>
#include <redis/request.hpp>

#include <storages/redis/reply_types.hpp>

namespace storages {
namespace redis {
namespace {

const std::string kOk{"OK"};
const std::string kPong{"PONG"};

const std::string& GetDescription(const ::redis::ReplyPtr& reply,
                                  const std::string& request_description) {
  return request_description.empty() ? reply->cmd : request_description;
}

const std::string& GetStringElem(const ::redis::ReplyData::Array& array,
                                 size_t elem_idx,
                                 const ::redis::ReplyPtr& reply,
                                 const std::string& request_description) {
  const auto& elem = array.at(elem_idx);
  if (!elem.IsString()) {
    const std::string& description = GetDescription(reply, request_description);
    throw ::redis::ParseReplyException(
        "Unexpected redis reply type to '" + description +
        "' request: " + "array[" + std::to_string(elem_idx) + "]: expected " +
        ::redis::ReplyData::TypeToString(::redis::ReplyData::Type::kString) +
        ", got type=" + elem.GetTypeString() + " elem=" + elem.ToString() +
        " msg=" + reply->data.ToString());
  }
  return elem.GetString();
}

}  // namespace

template <>
std::string ParseReply<std::string>(const ::redis::ReplyPtr& reply,
                                    const std::string& request_description) {
  reply->ExpectString(request_description);
  return reply->data.GetString();
}

template <>
boost::optional<std::string> ParseReply<boost::optional<std::string>>(
    const ::redis::ReplyPtr& reply, const std::string& request_description) {
  if (reply->data.IsNil()) return boost::none;
  return ParseReply<std::string>(reply, request_description);
}

template <>
double ParseReply<double>(const ::redis::ReplyPtr& reply,
                          const std::string& request_description) {
  reply->ExpectString(request_description);
  try {
    return std::stod(reply->data.GetString());
  } catch (const std::exception& ex) {
    throw ::redis::ParseReplyException(
        "Can't parse value from reply to '" +
        GetDescription(reply, request_description) + "' request (" +
        reply->data.ToString() + "): " + ex.what());
  }
}

template <>
boost::optional<double> ParseReply<boost::optional<double>>(
    const ::redis::ReplyPtr& reply, const std::string& request_description) {
  if (reply->data.IsNil()) return boost::none;
  reply->ExpectString(request_description);
  return ParseReply<double>(reply, request_description);
}

template <>
size_t ParseReply<size_t>(const ::redis::ReplyPtr& reply,
                          const std::string& request_description) {
  reply->ExpectInt(request_description);
  return reply->data.GetInt();
}

template <>
bool ParseReply<size_t, bool>(const ::redis::ReplyPtr& reply,
                              const std::string& request_description) {
  reply->ExpectInt(request_description);
  return !!reply->data.GetInt();
}

template <>
int64_t ParseReply<int64_t>(const ::redis::ReplyPtr& reply,
                            const std::string& request_description) {
  reply->ExpectInt(request_description);
  return reply->data.GetInt();
}

template <>
HsetReply ParseReply<HsetReply>(const ::redis::ReplyPtr& reply,
                                const std::string& request_description) {
  reply->ExpectInt(request_description);
  auto result = reply->data.GetInt();
  if (result < 0 || result > 1)
    throw ::redis::ParseReplyException(
        "Unexpected reply to '" + GetDescription(reply, request_description) +
        "' request: " + std::to_string(result));
  return result ? HsetReply::kCreated : HsetReply::kUpdated;
}

template <>
ExpireReply ParseReply<ExpireReply>(const ::redis::ReplyPtr& reply,
                                    const std::string& request_description) {
  return ExpireReply::Parse(reply, request_description);
}

template <>
TtlReply ParseReply<TtlReply>(const ::redis::ReplyPtr& reply,
                              const std::string& request_description) {
  return TtlReply::Parse(reply, request_description);
}

template <>
PersistReply ParseReply<PersistReply>(const ::redis::ReplyPtr& reply,
                                      const std::string& request_description) {
  reply->ExpectInt(request_description);
  auto value = reply->data.GetInt();
  switch (value) {
    case 0:
      return PersistReply::kKeyOrTimeoutNotFound;
    case 1:
      return PersistReply::kTimeoutRemoved;
    default:
      throw ::redis::ParseReplyException(
          "Unexpected reply to '" + GetDescription(reply, request_description) +
          "' request: " + std::to_string(value));
  }
}

template <>
KeyType ParseReply<KeyType>(const ::redis::ReplyPtr& reply,
                            const std::string& request_description) {
  reply->ExpectStatus(request_description);
  const auto& status = reply->data.GetStatus();
  try {
    return ParseKeyType(status);
  } catch (const std::exception& ex) {
    const std::string& description = GetDescription(reply, request_description);
    throw ::redis::ParseReplyException("Unexpected redis reply to '" +
                                       description + "' request. " + ex.what());
  }
}

template <>
void ParseReply<StatusOk, void>(const ::redis::ReplyPtr& reply,
                                const std::string& request_description) {
  reply->ExpectStatusEqualTo(kOk, request_description);
}

template <>
bool ParseReply<boost::optional<StatusOk>, bool>(
    const ::redis::ReplyPtr& reply, const std::string& request_description) {
  if (reply->data.IsNil()) return false;
  reply->ExpectStatusEqualTo(kOk, request_description);
  return true;
}

template <>
void ParseReply<StatusPong, void>(const ::redis::ReplyPtr& reply,
                                  const std::string& request_description) {
  reply->ExpectStatusEqualTo(kPong, request_description);
}

template <>
SetReply ParseReply<SetReply>(const ::redis::ReplyPtr& reply,
                              const std::string& request_description) {
  if (reply->data.IsNil()) return SetReply::kNotSet;
  reply->ExpectStatusEqualTo(kOk, request_description);
  return SetReply::kSet;
}

template <>
std::unordered_set<std::string> ParseReply<std::unordered_set<std::string>>(
    const ::redis::ReplyPtr& reply, const std::string& request_description) {
  reply->ExpectArray(request_description);

  const auto& array = reply->data.GetArray();
  std::unordered_set<std::string> result;
  result.reserve(array.size());

  for (size_t elem_idx = 0; elem_idx < array.size(); ++elem_idx) {
    result.emplace(GetStringElem(array, elem_idx, reply, request_description));
  }
  return result;
}

template <>
std::vector<std::string> ParseReply<std::vector<std::string>>(
    const ::redis::ReplyPtr& reply, const std::string& request_description) {
  reply->ExpectArray(request_description);

  const auto& array = reply->data.GetArray();
  std::vector<std::string> result;
  result.reserve(array.size());

  for (size_t elem_idx = 0; elem_idx < array.size(); ++elem_idx) {
    result.emplace_back(
        GetStringElem(array, elem_idx, reply, request_description));
  }
  return result;
}

template <>
std::vector<boost::optional<std::string>>
ParseReply<std::vector<boost::optional<std::string>>>(
    const ::redis::ReplyPtr& reply, const std::string& request_description) {
  reply->ExpectArray(request_description);

  const auto& array = reply->data.GetArray();
  std::vector<boost::optional<std::string>> result;
  result.reserve(array.size());

  for (size_t elem_idx = 0; elem_idx < array.size(); ++elem_idx) {
    const auto& elem = array[elem_idx];
    if (elem.IsNil()) {
      result.emplace_back(boost::none);
      continue;
    }
    result.emplace_back(
        GetStringElem(array, elem_idx, reply, request_description));
  }
  return result;
}

namespace {

::redis::ReplyData::KeyValues GetKeyValues(
    const ::redis::ReplyPtr& reply, const std::string& request_description) {
  try {
    return reply->data.GetKeyValues();
  } catch (const std::exception& ex) {
    const auto& description = GetDescription(reply, request_description);
    throw ::redis::ParseReplyException("Can't parse response to '" +
                                       description + "' request: " + ex.what());
  }
}

}  // namespace

template <>
std::vector<MemberScore> ParseReply<std::vector<MemberScore>>(
    const ::redis::ReplyPtr& reply, const std::string& request_description) {
  reply->ExpectIsOk(request_description);

  auto key_values = GetKeyValues(reply, request_description);

  std::vector<MemberScore> result;

  result.reserve(key_values.size());

  for (const auto elem : key_values) {
    const auto& member_elem = elem.Key();
    const auto& score_elem = elem.Value();
    double score;
    try {
      score = std::stod(score_elem);
    } catch (const std::exception& ex) {
      const auto& description = GetDescription(reply, request_description);
      throw ::redis::ParseReplyException(
          std::string("Can't parse response to '")
              .append(description)
              .append("' request: can't parse score from '")
              .append(score_elem)
              .append("' msg=")
              .append(reply->data.ToString())
              .append(": ")
              .append(ex.what()));
    }

    result.push_back({member_elem, score});
  }
  return result;
}

template <>
std::unordered_map<std::string, std::string>
ParseReply<std::unordered_map<std::string, std::string>>(
    const ::redis::ReplyPtr& reply, const std::string& request_description) {
  reply->ExpectIsOk(request_description);

  auto key_values = GetKeyValues(reply, request_description);

  std::unordered_map<std::string, std::string> result;

  result.reserve(key_values.size());

  for (const auto elem : key_values) {
    result[elem.Key()] = elem.Value();
  }
  return result;
}

template <>
::redis::ReplyData ParseReply<::redis::ReplyData>(
    const ::redis::ReplyPtr& reply, const std::string& request_description) {
  reply->ExpectIsOk(request_description);
  return reply->data;
}

template <>
ScanReply ParseReply<ScanReply>(const ::redis::ReplyPtr& reply,
                                const std::string& request_description) {
  return ScanReply::Parse(reply, request_description);
}

}  // namespace redis
}  // namespace storages
