#include <userver/storages/rocks/raw_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::rocks {

RawMap RawMap::FromSnapshot(Snapshot snapshot) { return RawMap{std::move(snapshot)}; }

RawMap::RawMap(Snapshot snapshot)
    : snapshot_(std::move(snapshot))
{}

std::optional<std::string> RawMap::operator[](std::string_view key) const { return snapshot_.Get(key); }

}  // namespace storages::rocks

USERVER_NAMESPACE_END
