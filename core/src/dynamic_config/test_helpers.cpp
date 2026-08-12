#include <userver/dynamic_config/test_helpers.hpp>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config {

dynamic_config::Source GetDefaultSource() {
    static const auto kStorage = MakeDefaultStorage({});
    return kStorage.GetSource();
}

const dynamic_config::Snapshot& GetDefaultSnapshot() {
    static const auto kSnapshot = GetDefaultSource().GetSnapshot();
    return kSnapshot;
}

dynamic_config::StorageMock MakeDefaultStorage(const std::vector<dynamic_config::KeyValue>& overrides) {
    return {impl::GetDefaultDocsMap(), overrides};
}

namespace impl {

const dynamic_config::DocsMap& GetDefaultDocsMap() {
    static const auto kDefaultDocsMap = dynamic_config::impl::MakeDefaultDocsMap();
    return kDefaultDocsMap;
}

}  // namespace impl

}  // namespace dynamic_config

USERVER_NAMESPACE_END
