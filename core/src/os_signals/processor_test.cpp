#include <userver/os_signals/processor_mock.hpp>

#include <utility>

#include <userver/utest/utest.hpp>
#include <userver/utils/resource_scopes.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

struct Helper {
    Helper(utils::ResourceScopeStorage& scopes, os_signals::Processor& proc, int signum) {
        proc.AddListener(scopes, this, "helper", signum, &Helper::OnSigUsr1);
    }

    Helper(utils::ResourceScopeStorage& scopes, os_signals::Processor& proc) {
        proc.AddListener(scopes, this, "helper", &Helper::OnSigUsrX);
    }

    void OnSigUsr1() { ++sigusr1; }

    void OnSigUsrX(int signum) {
        if (signum == os_signals::kSigUsr1) {
            ++sigusr1;
        } else {
            ASSERT_EQ(signum, os_signals::kSigUsr2);
            ++sigusr2;
        }
    }

    std::size_t sigusr1{0};
    std::size_t sigusr2{0};
};

}  // namespace

UTEST(OsSignals, Basic) {
    os_signals::ProcessorMock proc{engine::current_task::GetTaskProcessor()};
    UASSERT_NO_THROW(proc.Notify(os_signals::kSigUsr1));
    UASSERT_NO_THROW(proc.Notify(os_signals::kSigUsr2));

    utils::WithResourceScopes<Helper> h1(std::in_place, proc.Get(), os_signals::kSigUsr1);
    utils::WithResourceScopes<Helper> h2(std::in_place, proc.Get(), os_signals::kSigUsr1);

    EXPECT_EQ(h1->sigusr1, 0);
    EXPECT_EQ(h1->sigusr2, 0);
    EXPECT_EQ(h2->sigusr1, 0);
    EXPECT_EQ(h2->sigusr2, 0);

    proc.Notify(os_signals::kSigUsr1);
    EXPECT_EQ(h1->sigusr1, 1);
    EXPECT_EQ(h1->sigusr2, 0);
    EXPECT_EQ(h2->sigusr1, 1);
    EXPECT_EQ(h2->sigusr2, 0);

    proc.Notify(os_signals::kSigUsr1);
    EXPECT_EQ(h1->sigusr1, 2);
    EXPECT_EQ(h1->sigusr2, 0);
    EXPECT_EQ(h2->sigusr1, 2);
    EXPECT_EQ(h2->sigusr2, 0);

    // We were not subscribed to os_signals::kSigUsr2
    proc.Notify(os_signals::kSigUsr2);
    EXPECT_EQ(h1->sigusr1, 2);
    EXPECT_EQ(h1->sigusr2, 0);
    EXPECT_EQ(h2->sigusr1, 2);
    EXPECT_EQ(h2->sigusr2, 0);
}

UTEST(OsSignals, BothSignals) {
    os_signals::ProcessorMock proc{engine::current_task::GetTaskProcessor()};
    UASSERT_NO_THROW(proc.Notify(os_signals::kSigUsr1));
    UASSERT_NO_THROW(proc.Notify(os_signals::kSigUsr2));

    utils::WithResourceScopes<Helper> h1(std::in_place, proc.Get(), os_signals::kSigUsr1);
    utils::WithResourceScopes<Helper> h2(std::in_place, proc.Get());

    EXPECT_EQ(h1->sigusr1, 0);
    EXPECT_EQ(h1->sigusr2, 0);
    EXPECT_EQ(h2->sigusr1, 0);
    EXPECT_EQ(h2->sigusr2, 0);

    proc.Notify(os_signals::kSigUsr1);
    EXPECT_EQ(h1->sigusr1, 1);
    EXPECT_EQ(h1->sigusr2, 0);
    EXPECT_EQ(h2->sigusr1, 1);
    EXPECT_EQ(h2->sigusr2, 0);

    proc.Notify(os_signals::kSigUsr1);
    EXPECT_EQ(h1->sigusr1, 2);
    EXPECT_EQ(h1->sigusr2, 0);
    EXPECT_EQ(h2->sigusr1, 2);
    EXPECT_EQ(h2->sigusr2, 0);

    proc.Notify(os_signals::kSigUsr2);
    EXPECT_EQ(h1->sigusr1, 2);
    EXPECT_EQ(h1->sigusr2, 0);  // h1 is not subscribed to os_signals::kSigUsr2
    EXPECT_EQ(h2->sigusr1, 2);
    EXPECT_EQ(h2->sigusr2, 1);

    proc.Notify(os_signals::kSigUsr2);
    EXPECT_EQ(h1->sigusr1, 2);
    EXPECT_EQ(h1->sigusr2, 0);  // h1 is not subscribed to os_signals::kSigUsr2
    EXPECT_EQ(h2->sigusr1, 2);
    EXPECT_EQ(h2->sigusr2, 2);

    proc.Notify(os_signals::kSigUsr1);
    EXPECT_EQ(h1->sigusr1, 3);
    EXPECT_EQ(h1->sigusr2, 0);
    EXPECT_EQ(h2->sigusr1, 3);
    EXPECT_EQ(h2->sigusr2, 2);
}

USERVER_NAMESPACE_END
