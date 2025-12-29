#include <storages/mongo/cdriver/find_and_modify.hpp>

#include <formats/bson/wrappers.hpp>
#include <userver/storages/mongo/exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl::cdriver {

impl::cdriver::FindAndModifyOptsPtr CopyFindAndModifyOptions(const impl::cdriver::FindAndModifyOptsPtr& options) {
    impl::cdriver::FindAndModifyOptsPtr result(mongoc_find_and_modify_opts_new());

    {
        formats::bson::impl::UninitializedBson tmp_bson;
        mongoc_find_and_modify_opts_get_sort(options.get(), tmp_bson.Get());
        if (!mongoc_find_and_modify_opts_set_sort(result.get(), tmp_bson.Get())) {
            throw MongoException("Cannot set 'sort'");
        }
    }

    auto flags = mongoc_find_and_modify_opts_get_flags(options.get());
    if (!mongoc_find_and_modify_opts_set_flags(result.get(), flags)) {
        throw MongoException("Cannot set 'flags'");
    }

    if (!(flags & MONGOC_FIND_AND_MODIFY_REMOVE)) {
        formats::bson::impl::UninitializedBson tmp_bson;
        mongoc_find_and_modify_opts_get_update(options.get(), tmp_bson.Get());
        if (!mongoc_find_and_modify_opts_set_update(result.get(), tmp_bson.Get())) {
            throw MongoException("Cannot set 'update'");
        }
    }

    {
        formats::bson::impl::UninitializedBson tmp_bson;
        mongoc_find_and_modify_opts_get_fields(options.get(), tmp_bson.Get());
        if (!mongoc_find_and_modify_opts_set_fields(result.get(), tmp_bson.Get())) {
            throw MongoException("Cannot set 'fields'");
        }
    }

    {
        auto bypass_document_validation = mongoc_find_and_modify_opts_get_bypass_document_validation(options.get());
        if (!mongoc_find_and_modify_opts_set_bypass_document_validation(result.get(), bypass_document_validation)) {
            throw MongoException("Cannot set 'bypass_document_validation'");
        }
    }

    {
        auto max_time_ms = mongoc_find_and_modify_opts_get_max_time_ms(options.get());
        if (!mongoc_find_and_modify_opts_set_max_time_ms(result.get(), max_time_ms)) {
            throw MongoException("Cannot set 'max_time_ms'");
        }
    }

    {
        formats::bson::impl::UninitializedBson tmp_bson;
        mongoc_find_and_modify_opts_get_extra(options.get(), tmp_bson.Get());
        if (!mongoc_find_and_modify_opts_append(result.get(), tmp_bson.Get())) {
            throw MongoException("Cannot set 'extra'");
        }
    }

    return result;
}

}  // namespace storages::mongo::impl::cdriver

USERVER_NAMESPACE_END
