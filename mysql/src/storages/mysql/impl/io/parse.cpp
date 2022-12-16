#include <userver/storages/mysql/impl/io/parse.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::io {

void FieldParser<std::string>::Parse(std::string& source) {
  field_ = std::move(source);
}

void FieldParser<int>::Parse(std::string& source) {
  field_ = std::stoi(source);
}

}  // namespace storages::mysql::impl::io

USERVER_NAMESPACE_END
