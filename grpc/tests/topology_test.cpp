#include <gtest/gtest.h>

#include <ugrpc/impl/topology_sort.hpp>

#include <userver/utest/assert_macros.hpp>

USERVER_NAMESPACE_BEGIN

TEST(TopologySort, Basic) {
    std::unordered_map<std::string, std::vector<std::string>> graph{
        {"grpc-server-baggage", {"grpc-server-logging"}},
        {"grpc-server-congestion-control", {"grpc-server-logging"}},
        {"grpc-server-deadline-propagation", {"grpc-server-logging"}},
        {"grpc-server-headers-propagator", {"grpc-server-logging"}},
        {"grpc-server-logging", {}},
    };
    auto sort = ugrpc::middlewares::impl::BuildTopologySortOfMiddlewares(std::move(graph));
    std::vector<std::string> expected{
        "grpc-server-logging",
        "grpc-server-baggage",
        "grpc-server-congestion-control",
        "grpc-server-deadline-propagation",
        "grpc-server-headers-propagator"};
    ASSERT_EQ(sort, expected);
}

TEST(TopologySort, Throw) {
    /*
            <- B
          /    ^
     A <-      |
          \    |
            <- D
      C (independent)

    */
    std::unordered_map<std::string, std::vector<std::string>> graph{
        {"B", {"A"}},
        {"D", {"A", "B"}},
    };
    UASSERT_THROW(ugrpc::middlewares::impl::BuildTopologySortOfMiddlewares(std::move(graph)), std::runtime_error);
    graph["A"] = {};
    graph["C"] = {};
    auto sort = ugrpc::middlewares::impl::BuildTopologySortOfMiddlewares(std::move(graph));
    std::vector<std::string> expected{"A", "C", "B", "D"};
    ASSERT_EQ(sort, expected);
}

TEST(TopologySort, TwoSubPath) {
    /*
          <- B
         /
     A <-
         \
           <- C

     E <- D

    */
    std::unordered_map<std::string, std::vector<std::string>> graph{
        {"A", {}},
        {"E", {}},
        {"B", {"A"}},
        {"C", {"A"}},
        {"D", {"E"}},
    };
    auto sort = ugrpc::middlewares::impl::BuildTopologySortOfMiddlewares(std::move(graph));
    std::vector<std::string> expected{"A", "E", "B", "C", "D"};
    ASSERT_EQ(sort, expected);
}

USERVER_NAMESPACE_END
