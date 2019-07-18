#include <storages/redis/request_data_impl.hpp>

#include <unordered_map>

#include <redis/reply.hpp>

namespace storages {
namespace redis {
namespace {

const std::string kOk{"OK"};
const std::string kPong{"PONG"};

}  // namespace

namespace impl {
namespace {

::redis::ReplyPtr GetReply(::redis::Request& request) { return request.Get(); }

}  // namespace

void Wait(::redis::Request& request) {
  try {
    GetReply(request);
  } catch (const std::exception& ex) {
    LOG_WARNING() << "exception in GetReply(): " << ex;
  }
}

}  // namespace impl

RequestDataImplBase::RequestDataImplBase(::redis::Request&& request)
    : request_(std::move(request)) {}

RequestDataImplBase::~RequestDataImplBase() = default;

::redis::ReplyPtr RequestDataImplBase::GetReply() {
  return impl::GetReply(request_);
}

::redis::Request& RequestDataImplBase::GetRequest() { return request_; }

template <>
boost::optional<std::string> RequestDataImpl<boost::optional<std::string>>::Get(
    const std::string& request_description) {
  auto reply = GetReply();
  if (reply->data.IsNil()) return boost::none;
  reply->ExpectString(request_description);
  return reply->data.GetString();
}

template <>
boost::optional<double> RequestDataImpl<boost::optional<double>>::Get(
    const std::string& request_description) {
  auto reply = GetReply();
  if (reply->data.IsNil()) return boost::none;
  reply->ExpectString(request_description);
  try {
    return std::stod(reply->data.GetString());
  } catch (const std::exception& ex) {
    // TODO: use custom types for exceptions
    throw ::redis::ParseReplyException(
        "Can't parse value from reply to '" + request_description +
        "' request (" + reply->data.ToString() + "): " + ex.what());
  }
}

template <>
size_t RequestDataImpl<size_t>::Get(const std::string& request_description) {
  auto reply = GetReply();
  reply->ExpectInt(request_description);
  return reply->data.GetInt();
}

template <>
HsetReply RequestDataImpl<HsetReply>::Get(
    const std::string& request_description) {
  auto reply = GetReply();
  reply->ExpectInt(request_description);
  auto result = reply->data.GetInt();
  if (result < 0 || result > 1)
    throw ::redis::ParseReplyException("unexpected Hset reply: " +
                                       std::to_string(result));
  return result ? HsetReply::kCreated : HsetReply::kUpdated;
}

template <>
TtlReply RequestDataImpl<TtlReply>::Get(
    const std::string& request_description) {
  return TtlReply::Parse(GetReply(), request_description);
}

template <>
TypeReply RequestDataImpl<TypeReply>::Get(
    const std::string& request_description) {
  static const std::unordered_map<std::string, TypeReply> types_map{
      {"string", TypeReply::kString}, {"list", TypeReply::kList},
      {"set", TypeReply::kSet},       {"zset", TypeReply::kZset},
      {"hash", TypeReply::kHash},     {"stream", TypeReply::kStream}};
  auto reply = GetReply();
  reply->ExpectStatus(request_description);
  const auto& status = reply->data.GetStatus();
  auto it = types_map.find(status);
  if (it == types_map.end()) {
    const std::string& request =
        request_description.empty() ? reply->cmd : request_description;
    throw ::redis::ParseReplyException("unexpected redis reply to '" + request +
                                       "' request. unknown type: '" + status +
                                       '\'');
  }
  return it->second;
}

template <>
std::string RequestDataImpl<std::string>::Get(
    const std::string& request_description) {
  auto reply = GetReply();
  reply->ExpectString(request_description);
  return reply->data.GetString();
}

template <>
void RequestDataImpl<StatusOk, void>::Get(
    const std::string& request_description) {
  auto reply = GetReply();
  reply->ExpectStatusEqualTo(kOk, request_description);
}

template <>
void RequestDataImpl<StatusPong, void>::Get(
    const std::string& request_description) {
  auto reply = GetReply();
  reply->ExpectStatusEqualTo(kPong, request_description);
}

template <>
SetReply RequestDataImpl<SetReply>::Get(
    const std::string& request_description) {
  auto reply = GetReply();
  if (reply->data.IsNil()) return SetReply::kNotSet;
  reply->ExpectStatusEqualTo(kOk, request_description);
  return SetReply::kSet;
}

template <>
std::vector<std::string> RequestDataImpl<std::vector<std::string>>::Get(
    const std::string& request_description) {
  auto reply = GetReply();
  reply->ExpectArray(request_description);

  const auto& array = reply->data.GetArray();
  std::vector<std::string> result;
  result.reserve(array.size());

  for (size_t elem_idx = 0; elem_idx < array.size(); ++elem_idx) {
    const auto& elem = array[elem_idx];
    if (!elem.IsString()) {
      const std::string& request =
          request_description.empty() ? reply->cmd : request_description;
      throw ::redis::ParseReplyException(
          "unexpected redis reply type to '" + request +
          "' request: " + "array[" + std::to_string(elem_idx) + "]: expected " +
          ::redis::ReplyData::TypeToString(::redis::ReplyData::Type::kString) +
          ", got type=" + elem.GetTypeString() + " elem=" + elem.ToString() +
          " msg=" + reply->data.ToString());
    }
    result.emplace_back(elem.GetString());
  }
  return result;
}

template <>
std::vector<boost::optional<std::string>>
RequestDataImpl<std::vector<boost::optional<std::string>>>::Get(
    const std::string& request_description) {
  auto reply = GetReply();
  reply->ExpectArray(request_description);

  const auto& array = reply->data.GetArray();
  std::vector<boost::optional<std::string>> result;
  result.reserve(array.size());

  for (size_t elem_idx = 0; elem_idx < array.size(); ++elem_idx) {
    const auto& elem = array[elem_idx];
    if (elem.IsNil()) {
      result.emplace_back(boost::none);
    } else if (!elem.IsString()) {
      const std::string& request =
          request_description.empty() ? reply->cmd : request_description;
      throw ::redis::ParseReplyException(
          "unexpected redis reply type to '" + request +
          "' request: " + "array[" + std::to_string(elem_idx) + "]: expected " +
          ::redis::ReplyData::TypeToString(::redis::ReplyData::Type::kString) +
          ", got type=" + elem.GetTypeString() + " elem=" + elem.ToString() +
          " msg=" + reply->data.ToString());
    }
    result.emplace_back(elem.GetString());
  }
  return result;
}

namespace {

::redis::ReplyData::KeyValues GetKeyValues(const ::redis::ReplyPtr& reply,
                                           const std::string& request) {
  try {
    return reply->data.GetKeyValues();
  } catch (const std::exception& ex) {
    throw ::redis::ParseReplyException("can't parse response to '" + request +
                                       "' request: " + ex.what());
  }
}

}  // namespace

template <>
std::vector<MemberScore> RequestDataImpl<std::vector<MemberScore>>::Get(
    const std::string& request_description) {
  auto reply = GetReply();
  const std::string& request =
      request_description.empty() ? reply->cmd : request_description;

  auto key_values = GetKeyValues(reply, request);

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
          std::string("can't parse response to '")
              .append(request)
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
RequestDataImpl<std::unordered_map<std::string, std::string>>::Get(
    const std::string& request_description) {
  auto reply = GetReply();
  const std::string& request =
      request_description.empty() ? reply->cmd : request_description;

  auto key_values = GetKeyValues(reply, request);

  std::unordered_map<std::string, std::string> result;

  result.reserve(key_values.size());

  for (const auto elem : key_values) {
    result[elem.Key()] = elem.Value();
  }
  return result;
}

}  // namespace redis
}  // namespace storages
