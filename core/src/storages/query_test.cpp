#include <userver/storages/query.hpp>

#include <userver/compiler/impl/constexpr.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr const char* kStatement = "The query that is too long to fit into SSO because it is too long";
constexpr const char* kQueryName = "query_name_that_does_not_fir_into_SSO_because_it_is_too_big";

// Validate that variable initializes without dynamic initialization
USERVER_IMPL_CONSTINIT const storages::Query kStaticInit{
    kStatement,
    storages::Query::NameLiteral{kQueryName},
    storages::Query::LogMode::kFull,
};

// Validate that variable initializes without dynamic initialization
USERVER_IMPL_CONSTINIT const storages::Query kEmptyStaticInit{};

}  // namespace

TEST(Query, StaticInit) {
    EXPECT_EQ(kStaticInit.GetStatementView(), kStatement);
    EXPECT_EQ(kStaticInit.GetStatementView().c_str(), kStatement);
    EXPECT_EQ(kStaticInit.GetOptionalNameView(), kQueryName);
    EXPECT_EQ(kStaticInit.GetOptionalNameView()->c_str(), kQueryName);

    auto other = kStaticInit;
    EXPECT_EQ(other.GetStatementView(), kStatement);
    EXPECT_EQ(other.GetStatementView().c_str(), kStatement);
    EXPECT_EQ(other.GetOptionalNameView(), kQueryName);
    EXPECT_EQ(other.GetOptionalNameView()->c_str(), kQueryName);

    auto other2 = std::move(other);
    EXPECT_EQ(other2.GetStatementView(), kStatement);
    EXPECT_EQ(other2.GetStatementView().c_str(), kStatement);
    EXPECT_EQ(other2.GetOptionalNameView(), kQueryName);
    EXPECT_EQ(other2.GetOptionalNameView()->c_str(), kQueryName);

    other2 = kStaticInit;
    EXPECT_EQ(other2.GetStatementView(), kStatement);
    EXPECT_EQ(other2.GetStatementView().c_str(), kStatement);
    EXPECT_EQ(other2.GetOptionalNameView(), kQueryName);
    EXPECT_EQ(other2.GetOptionalName()->GetUnderlying(), kQueryName);
    EXPECT_EQ(other2.GetOptionalNameView()->c_str(), kQueryName);

    EXPECT_EQ(kStaticInit.GetLogMode(), storages::Query::LogMode::kFull);
}

TEST(Query, StaticInitEmpty) {
    EXPECT_EQ(kEmptyStaticInit.GetStatementView(), "");
    EXPECT_FALSE(kEmptyStaticInit.GetOptionalNameView());
}

TEST(Query, RunTime) {
    {
        storages::Query query{
            std::string{kStatement},
            storages::Query::Name{kQueryName},
        };

        EXPECT_EQ(query.GetStatementView(), kStatement);
        EXPECT_NE(query.GetStatementView().c_str(), kStatement);
        EXPECT_EQ(query.GetOptionalNameView(), kQueryName);
        EXPECT_NE(query.GetOptionalNameView()->c_str(), kQueryName);

        auto query_copy = query;
        auto query_copy2 = std::move(query);
        auto query_copy3 = query_copy2;
        EXPECT_EQ(query_copy3.GetStatementView(), kStatement);
        EXPECT_NE(query_copy3.GetStatementView().c_str(), kStatement);
        EXPECT_EQ(query_copy3.GetOptionalNameView(), kQueryName);
        EXPECT_NE(query_copy3.GetOptionalNameView()->c_str(), kQueryName);

        query = kStaticInit;
        EXPECT_EQ(query.GetStatementView(), kStatement);
        EXPECT_EQ(query.GetStatementView().c_str(), kStatement);
        EXPECT_EQ(query.GetOptionalNameView(), kQueryName);
        EXPECT_EQ(query.GetOptionalNameView()->c_str(), kQueryName);

        query = query_copy;
        EXPECT_EQ(query.GetStatementView(), kStatement);
        EXPECT_NE(query.GetStatementView().c_str(), kStatement);
        EXPECT_EQ(query.GetOptionalNameView(), kQueryName);
        EXPECT_NE(query.GetOptionalNameView()->c_str(), kQueryName);

        query = std::move(query_copy2);
        EXPECT_EQ(query.GetStatementView(), kStatement);
        EXPECT_EQ(query.GetOptionalNameView(), kQueryName);
    }
    {
        storages::Query query{
            kStatement,
            storages::Query::Name{kQueryName},
        };

        EXPECT_EQ(query.GetStatementView(), kStatement);
        EXPECT_EQ(query.GetOptionalNameView(), kQueryName);
    }
}

TEST(Query, SelfConstruct) {
    storages::Query query{
        std::string{kStaticInit.GetStatementView()},
        kStaticInit.GetOptionalName(),
    };
    EXPECT_EQ(query.GetStatementView(), kStatement);
    EXPECT_EQ(query.GetOptionalNameView(), kQueryName);
}

TEST(Query, OneLiteral) {
    const storages::Query query{
        "statement",
        storages::Query::Name{"name"},
    };
    EXPECT_EQ(query.GetStatementView(), "statement");
    EXPECT_EQ(query.GetOptionalNameView(), "name");
}

TEST(Query, FmtRuntime) {
    const storages::Query query{
        "statement {}",
        storages::Query::Name{"name"},
    };
    EXPECT_EQ(fmt::format(fmt::runtime(query.GetStatementView()), "OK"), "statement OK");
}

USERVER_NAMESPACE_END
