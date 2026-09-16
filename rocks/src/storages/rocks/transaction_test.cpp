#include <userver/storages/rocks/client.hpp>

#include <userver/fs/blocking/temp_directory.hpp>
#include <userver/storages/rocks/exception.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using fs::blocking::TempDirectory;
using storages::rocks::Transaction;
using storages::rocks::TransactionType;
using storages::rocks::WriteConflictException;

UTEST(PessimisticTxn, CommitPersists) {
    const auto dir = TempDirectory::Create();
    storages::rocks::Client client{
        dir.GetPath(),
        TransactionType::kPessimistic,
    };

    auto txn = client.BeginTransaction();
    txn.Put("k", "v");
    txn.Commit();

    EXPECT_EQ("v", client.Get("k"));
}

UTEST(PessimisticTxn, RollbackDiscards) {
    const auto dir = TempDirectory::Create();
    storages::rocks::Client client{
        dir.GetPath(),
        TransactionType::kPessimistic,
    };

    auto txn = client.BeginTransaction();
    txn.Put("k", "v");
    txn.Rollback();

    EXPECT_EQ(std::nullopt, client.Get("k"));
}

UTEST(PessimisticTxn, DestructorRollsBack) {
    const auto dir = TempDirectory::Create();
    storages::rocks::Client client{
        dir.GetPath(),
        TransactionType::kPessimistic,
    };

    {
        auto txn = client.BeginTransaction();
        txn.Put("k", "v");
        // txn destroyed without Commit() — must roll back
    }

    EXPECT_EQ(std::nullopt, client.Get("k"));
}

UTEST(PessimisticTxn, ReadOwnWrites) {
    const auto dir = TempDirectory::Create();
    storages::rocks::Client client{
        dir.GetPath(),
        TransactionType::kPessimistic,
    };

    auto txn = client.BeginTransaction();
    txn.Put("k", "v");
    EXPECT_EQ("v", txn.Get("k"));
    // not yet visible outside
    EXPECT_EQ(std::nullopt, client.Get("k"));
    txn.Commit();
}

UTEST(PessimisticTxn, SavePoint) {
    const auto dir = TempDirectory::Create();
    storages::rocks::Client client{
        dir.GetPath(),
        TransactionType::kPessimistic,
    };

    auto txn = client.BeginTransaction();
    txn.Put("a", "1");
    txn.SetSavePoint();
    txn.Put("b", "2");
    txn.RollbackToSavePoint();
    txn.Commit();

    EXPECT_EQ("1", client.Get("a"));
    EXPECT_EQ(std::nullopt, client.Get("b"));
}

UTEST(PessimisticTxn, SurvivesClientDestruction) {
    const auto dir = TempDirectory::Create();

    Transaction txn = [&] {
        storages::rocks::Client client{
            dir.GetPath(),
            TransactionType::kPessimistic,
        };
        client.Put("k", "before");
        auto t = client.BeginTransaction();
        t.Put("k", "txn");
        return t;
    }();

    txn.Commit();
}

UTEST(OptimisticTxn, CommitPersists) {
    const auto dir = TempDirectory::Create();
    storages::rocks::Client client{
        dir.GetPath(),
        TransactionType::kOptimistic,
    };

    auto txn = client.BeginTransaction();
    txn.Put("k", "v");
    txn.Commit();

    EXPECT_EQ("v", client.Get("k"));
}

UTEST(OptimisticTxn, NoConflictCommits) {
    const auto dir = TempDirectory::Create();
    storages::rocks::Client client{
        dir.GetPath(),
        TransactionType::kOptimistic,
    };

    auto txn1 = client.BeginTransaction();
    auto txn2 = client.BeginTransaction();

    txn1.Put("a", "1");
    txn2.Put("b", "2");

    txn1.Commit();
    txn2.Commit();

    EXPECT_EQ("1", client.Get("a"));
    EXPECT_EQ("2", client.Get("b"));
}

UTEST(OptimisticTxn, ConflictThrows) {
    const auto dir = TempDirectory::Create();
    storages::rocks::Client client{
        dir.GetPath(),
        TransactionType::kOptimistic,
    };

    auto txn1 = client.BeginTransaction();
    auto txn2 = client.BeginTransaction();

    txn1.GetForUpdate("k");
    txn2.GetForUpdate("k");

    txn1.Put("k", "from-txn1");
    txn2.Put("k", "from-txn2");

    txn1.Commit();

    EXPECT_THROW(txn2.Commit(), WriteConflictException);
}

}  // namespace

USERVER_NAMESPACE_END
