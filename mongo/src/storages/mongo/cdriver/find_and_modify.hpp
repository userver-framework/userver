#pragma once

#include <storages/mongo/cdriver/wrappers.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl::cdriver {

FindAndModifyOptsPtr CopyFindAndModifyOptions(const FindAndModifyOptsPtr& options);

}

USERVER_NAMESPACE_END
