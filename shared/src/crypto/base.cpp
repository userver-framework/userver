#include <userver/crypto/basic_types.hpp>

USERVER_NAMESPACE_BEGIN

namespace crypto {

NamedAlgo::NamedAlgo(std::string name) : name_(std::move(name)) {}
NamedAlgo::~NamedAlgo() = default;

const std::string& NamedAlgo::Name() const { return name_; }

}  // namespace crypto

USERVER_NAMESPACE_END
