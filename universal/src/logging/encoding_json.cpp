#include "encoding_json.hpp"

#include <rapidjson/writer.h>

USERVER_NAMESPACE_BEGIN

namespace logging {

namespace {
class LogBufferStreamWrapper {
 public:
  using Ch = LogBuffer::value_type;

  explicit LogBufferStreamWrapper(LogBuffer& lb, bool skip_first_quote)
      : lb_{lb}, skip_first_quote_{skip_first_quote} {}

  void Put(Ch ch) {
    if (!std::exchange(skip_first_quote_, false)) {
      lb_.push_back(ch);
    }
  }

  void Reserve(size_t count) { lb_.reserve(lb_.size() + count); }

  void Flush() {}

 private:
  LogBuffer& lb_;
  bool skip_first_quote_;
};

void PutReserve(LogBufferStreamWrapper& stream, size_t count) {
  stream.Reserve(count);
}
}  // namespace

void EncodeJson(LogBuffer& lb, std::string_view string) {
  // NOTE: rapidjson::Writer adds leading and trailing quotes
  LogBufferStreamWrapper output_stream(lb, /*skip_first_quote=*/true);
  rapidjson::Writer writer(output_stream);

  writer.String(string.data(), string.size());

  // skip last '""
  lb.resize(lb.size() - 1);
}

}  // namespace logging

USERVER_NAMESPACE_END
