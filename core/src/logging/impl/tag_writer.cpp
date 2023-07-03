#include <userver/logging/impl/tag_writer.hpp>

#include <boost/container/small_vector.hpp>

#include <utils/internal_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

namespace {

struct ExtraValue final {
  const LogExtra::Value& value;
};

LogHelper& operator<<(LogHelper& lh, ExtraValue value) noexcept {
  std::visit([&lh](const auto& contents) { lh << contents; }, value.value);
  return lh;
}

}  // namespace

void TagWriter::PutLogExtra(const LogExtra& extra) {
  for (const auto& item : *extra.extra_) {
    PutTag(item.first, ExtraValue{item.second.GetValue()});
  }
}

TagWriter::TagWriter(utils::InternalTag, LogHelper& lh) noexcept : lh_(lh) {}

}  // namespace logging::impl

USERVER_NAMESPACE_END
