#pragma once

#include <memory>

#include <cassandra.h>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla::impl::driver {

struct CassClusterDeleter {
    void operator()(CassCluster* c) const { cass_cluster_free(c); }
};

struct CassSessionDeleter {
    void operator()(CassSession* s) const { cass_session_free(s); }
};

struct CassFutureDeleter {
    void operator()(CassFuture* f) const { cass_future_free(f); }
};

struct CassStatementDeleter {
    void operator()(CassStatement* s) const { cass_statement_free(s); }
};

struct CassRetryPolicyDeleter {
    void operator()(CassRetryPolicy* p) const { cass_retry_policy_free(p); }
};

struct CassSslDeleter {
    void operator()(CassSsl* s) const { cass_ssl_free(s); }
};

struct CassPreparedDeleter {
    void operator()(const CassPrepared* p) const {
        if (p) {
            cass_prepared_free(p);
        }
    }
};

struct CassResultDeleter {
    void operator()(const CassResult* r) const {
        if (r) {
            cass_result_free(r);
        }
    }
};

struct CassIteratorDeleter {
    void operator()(CassIterator* it) const {
        if (it) {
            cass_iterator_free(it);
        }
    }
};

struct CassBatchDeleter {
    void operator()(CassBatch* b) const {
        if (b) {
            cass_batch_free(b);
        }
    }
};

struct CassCollectionDeleter {
    void operator()(CassCollection* c) const {
        if (c) {
            cass_collection_free(c);
        }
    }
};

using CassClusterPtr = std::unique_ptr<CassCluster, CassClusterDeleter>;
using CassSessionPtr = std::unique_ptr<CassSession, CassSessionDeleter>;
using CassFuturePtr = std::unique_ptr<CassFuture, CassFutureDeleter>;
using CassStatementPtr = std::unique_ptr<CassStatement, CassStatementDeleter>;
using CassRetryPolicyPtr = std::unique_ptr<CassRetryPolicy, CassRetryPolicyDeleter>;
using CassSslPtr = std::unique_ptr<CassSsl, CassSslDeleter>;
using CassPreparedPtr = std::unique_ptr<const CassPrepared, CassPreparedDeleter>;
using CassResultPtr = std::unique_ptr<const CassResult, CassResultDeleter>;
using CassIteratorPtr = std::unique_ptr<CassIterator, CassIteratorDeleter>;
using CassBatchPtr = std::unique_ptr<CassBatch, CassBatchDeleter>;
using CassCollectionPtr = std::unique_ptr<CassCollection, CassCollectionDeleter>;

}  // namespace storages::scylla::impl::driver

USERVER_NAMESPACE_END
