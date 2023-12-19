#include <userver/storages/mysql/impl/io/extractor.hpp>

#include <storages/mysql/impl/bindings/output_bindings.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::io {

ExtractorBase::ExtractorBase(std::size_t size) : binder_{size} {}

ExtractorBase::~ExtractorBase() = default;

void ExtractorBase::UpdateBinds(void* binds_array) {
  binder_.GetBinds().WrapBinds(binds_array);
}

}  // namespace storages::mysql::impl::io

USERVER_NAMESPACE_END
