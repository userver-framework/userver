#include <storages/postgres/io/integral_types.hpp>

#include <unordered_set>

namespace storages {
namespace postgres {
namespace io {

namespace {

const std::unordered_set<std::string> kTrueLiterals{"TRUE", "t",  "true", "y",
                                                    "yes",  "on", "1"};

const std::unordered_set<std::string> kFalseLiterals{
    "FALSE", "f", "false", "n", "no", "off", "0"};

}  // namespace

void BufferParser<bool, DataFormat::kTextDataFormat>::operator()(
    const FieldBuffer& buf) {
  std::string bool_literal(buf.buffer, buf.length);
  value = kTrueLiterals.count(bool_literal) > 0;
}

}  // namespace io
}  // namespace postgres
}  // namespace storages
