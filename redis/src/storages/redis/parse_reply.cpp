#include <storages/redis/parse_reply.hpp>

#include <redis/reply.hpp>
#include <redis/request.hpp>

namespace storages {
namespace redis {
namespace {

const std::string kOk{"OK"};
const std::string kPong{"PONG"};

const std::string& GetStringElem(const ::redis::ReplyData::Array& array,
                                 size_t elem_idx, const ReplyPtr& reply,
                                 const std::string& request_description) {
  const auto& elem = array.at(elem_idx);
  if (!elem.IsString()) {
    throw ::redis::ParseReplyException(
        "Unexpected redis reply type to '" + request_description +
        "' request: " + "array[" + std::to_string(elem_idx) + "]: expected " +
        ::redis::ReplyData::TypeToString(::redis::ReplyData::Type::kString) +
        ", got type=" + elem.GetTypeString() + " elem=" + elem.ToString() +
        " msg=" + reply->data.ToString());
  }
  return elem.GetString();
}

::redis::ReplyData::KeyValues GetKeyValues(
    const ::redis::ReplyData& array_data,
    const std::string& request_description) {
  try {
    return array_data.GetKeyValues();
  } catch (const std::exception& ex) {
    throw ::redis::ParseReplyException("Can't parse response to '" +
                                       request_description +
                                       "' request: " + ex.what());
  }
}

}  // namespace

namespace impl {

const ::redis::ReplyData& GetArrayData(const ReplyPtr& reply,
                                       const std::string& request_description) {
  reply->ExpectArray(request_description);
  return reply->data;
}

}  // namespace impl

std::vector<std::string> ParseReplyDataArray(
    const ::redis::ReplyData& array_data, const ReplyPtr& reply,
    const std::string& request_description, To<std::vector<std::string>>) {
  const auto& array = array_data.GetArray();
  std::vector<std::string> result;
  result.reserve(array.size());

  for (size_t elem_idx = 0; elem_idx < array.size(); ++elem_idx) {
    result.emplace_back(
        GetStringElem(array, elem_idx, reply, request_description));
  }
  return result;
}

std::vector<boost::optional<std::string>> ParseReplyDataArray(
    const ::redis::ReplyData& array_data, const ReplyPtr& reply,
    const std::string& request_description,
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
        GetStringElem(array, elem_idx, reply, request_description));
  }
  return result;
}

std::vector<std::pair<std::string, std::string>> ParseReplyDataArray(
    const ::redis::ReplyData& array_data, const ReplyPtr&,
    const std::string& request_description,
    To<std::vector<std::pair<std::string, std::string>>>) {
  auto key_values = GetKeyValues(array_data, request_description);

  std::vector<std::pair<std::string, std::string>> result;

  result.reserve(key_values.size());

  for (const auto elem : key_values) {
    result.emplace_back(elem.Key(), elem.Value());
  }
  return result;
}

std::vector<MemberScore> ParseReplyDataArray(
    const ::redis::ReplyData& array_data, const ReplyPtr& reply,
    const std::string& request_description, To<std::vector<MemberScore>>) {
  auto key_values = GetKeyValues(array_data, request_description);

  std::vector<MemberScore> result;

  result.reserve(key_values.size());

  for (const auto elem : key_values) {
    const auto& member_elem = elem.Key();
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
              .append("' msg=")
              .append(reply->data.ToString())
              .append(": ")
              .append(ex.what()));
    }

    result.emplace_back(member_elem, score);
  }
  return result;
}

std::string Parse(const ReplyPtr& reply, const std::string& request_description,
                  To<std::string>) {
  reply->ExpectString(request_description);
  return reply->data.GetString();
}

boost::optional<std::string> Parse(const ReplyPtr& reply,
                                   const std::string& request_description,
                                   To<boost::optional<std::string>>) {
  if (reply->data.IsNil()) return boost::none;
  return Parse(reply, request_description, To<std::string>{});
}

double Parse(const ReplyPtr& reply, const std::string& request_description,
             To<double>) {
  reply->ExpectString(request_description);
  try {
    return std::stod(reply->data.GetString());
  } catch (const std::exception& ex) {
    throw ::redis::ParseReplyException(
        "Can't parse value from reply to '" + request_description +
        "' request (" + reply->data.ToString() + "): " + ex.what());
  }
}

boost::optional<double> Parse(const ReplyPtr& reply,
                              const std::string& request_description,
                              To<boost::optional<double>>) {
  if (reply->data.IsNil()) return boost::none;
  reply->ExpectString(request_description);
  return Parse(reply, request_description, To<double>{});
}

size_t Parse(const ReplyPtr& reply, const std::string& request_description,
             To<size_t>) {
  reply->ExpectInt(request_description);
  return reply->data.GetInt();
}

bool Parse(const ReplyPtr& reply, const std::string& request_description,
           To<size_t, bool>) {
  reply->ExpectInt(request_description);
  return !!reply->data.GetInt();
}

int64_t Parse(const ReplyPtr& reply, const std::string& request_description,
              To<int64_t>) {
  reply->ExpectInt(request_description);
  return reply->data.GetInt();
}

HsetReply Parse(const ReplyPtr& reply, const std::string& request_description,
                To<HsetReply>) {
  reply->ExpectInt(request_description);
  auto result = reply->data.GetInt();
  if (result < 0 || result > 1)
    throw ::redis::ParseReplyException("Unexpected reply to '" +
                                       request_description +
                                       "' request: " + std::to_string(result));
  return result ? HsetReply::kCreated : HsetReply::kUpdated;
}

PersistReply Parse(const ReplyPtr& reply,
                   const std::string& request_description, To<PersistReply>) {
  reply->ExpectInt(request_description);
  auto value = reply->data.GetInt();
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

KeyType Parse(const ReplyPtr& reply, const std::string& request_description,
              To<KeyType>) {
  reply->ExpectStatus(request_description);
  const auto& status = reply->data.GetStatus();
  try {
    return ParseKeyType(status);
  } catch (const std::exception& ex) {
    throw ::redis::ParseReplyException("Unexpected redis reply to '" +
                                       request_description + "' request. " +
                                       ex.what());
  }
}

void Parse(const ReplyPtr& reply, const std::string& request_description,
           To<StatusOk, void>) {
  reply->ExpectStatusEqualTo(kOk, request_description);
}

bool Parse(const ReplyPtr& reply, const std::string& request_description,
           To<boost::optional<StatusOk>, bool>) {
  if (reply->data.IsNil()) return false;
  reply->ExpectStatusEqualTo(kOk, request_description);
  return true;
}

void Parse(const ReplyPtr& reply, const std::string& request_description,
           To<StatusPong, void>) {
  reply->ExpectStatusEqualTo(kPong, request_description);
}

SetReply Parse(const ReplyPtr& reply, const std::string& request_description,
               To<SetReply>) {
  if (reply->data.IsNil()) return SetReply::kNotSet;
  reply->ExpectStatusEqualTo(kOk, request_description);
  return SetReply::kSet;
}

std::unordered_set<std::string> Parse(const ReplyPtr& reply,
                                      const std::string& request_description,
                                      To<std::unordered_set<std::string>>) {
  reply->ExpectArray(request_description);

  const auto& array = reply->data.GetArray();
  std::unordered_set<std::string> result;
  result.reserve(array.size());

  for (size_t elem_idx = 0; elem_idx < array.size(); ++elem_idx) {
    result.emplace(GetStringElem(array, elem_idx, reply, request_description));
  }
  return result;
}

std::unordered_map<std::string, std::string> Parse(
    const ReplyPtr& reply, const std::string& request_description,
    To<std::unordered_map<std::string, std::string>>) {
  reply->ExpectArray(request_description);

  auto key_values = GetKeyValues(reply->data, request_description);

  std::unordered_map<std::string, std::string> result;

  result.reserve(key_values.size());

  for (const auto elem : key_values) {
    result[elem.Key()] = elem.Value();
  }
  return result;
}

::redis::ReplyData Parse(const ReplyPtr& reply,
                         const std::string& request_description,
                         To<::redis::ReplyData>) {
  reply->ExpectIsOk(request_description);
  return reply->data;
}

const std::string& RequestDescription(const ReplyPtr& reply,
                                      const std::string& request_description) {
  return request_description.empty() ? reply->cmd : request_description;
}

}  // namespace redis
}  // namespace storages
