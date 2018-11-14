#include <storages/postgres/io/integral_types.hpp>

#include <unordered_set>

namespace storages {
namespace postgres {
namespace io {

namespace {

bool IsTrueLiteral(const std::string& lit) {
  static const std::unordered_set<std::string> kTrueLiterals{
      "TRUE", "t", "true", "y", "yes", "on", "1"};
  return kTrueLiterals.count(lit);
}

const std::unordered_set<std::string> kFalseLiterals{
    "FALSE", "f", "false", "n", "no", "off", "0"};

}  // namespace

void BufferParser<bool, DataFormat::kTextDataFormat>::operator()(
    const FieldBuffer& buf) {
  std::string bool_literal(buf.buffer, buf.length);
  value = IsTrueLiteral(bool_literal);
}

}  // namespace io
}  // namespace postgres
}  // namespace storages
