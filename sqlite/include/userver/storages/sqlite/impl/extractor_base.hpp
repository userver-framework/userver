#pragma once

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

class ExtractorBase {
public:
    virtual ~ExtractorBase() = default;

    virtual void BindNextRow() = 0;
};

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
