#include <userver/storages/mysql/io/parse.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::io {

void FieldParser<std::string>::Parse(std::string& source) {
  field_ = std::move(source);
}

void FieldParser<int>::Parse(std::string& source) {
  field_ = std::stoi(source);
}

}  // namespace storages::mysql::io

USERVER_NAMESPACE_END
