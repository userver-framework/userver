#pragma once

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

struct FieldTag {};
struct RowTag {};

constexpr FieldTag kFieldTag;
constexpr RowTag kRowTag{};

}  // namespace storages::mysql

USERVER_NAMESPACE_END
