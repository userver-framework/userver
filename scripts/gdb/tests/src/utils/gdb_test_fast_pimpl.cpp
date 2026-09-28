#include <string>

#include <userver/gdb_tests/stub.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

struct ValueTest {
    std::string value;
};

struct ValueHolder {
    static constexpr std::size_t kNativeNodeSize = 32;
    static constexpr std::size_t kNativeAlignment = alignof(void*);
    utils::FastPimpl<ValueTest, kNativeNodeSize, kNativeAlignment> underlying;
};

}  // namespace

__attribute__((noinline)) static void TestGdbPrinters() {
    ValueHolder value;

    DoNotOptimize(value);

    value.underlying->value = "this is an underlying value";
    TEST_EXPR(
        'value',
        '{static kNativeNodeSize = 32, static kNativeAlignment = 8, underlying = {' + USERVER_NAMESPACE +
            'utils::FastPimpl<' + USERVER_NAMESPACE +
            '(anonymous namespace)::ValueTest, 32, 8, false>::storage_ = {value = "this is an underlying value"}}}',
    );

    DoNotOptimize(value);
    TEST_DEINIT(value);
}

USERVER_NAMESPACE_END

int main() { USERVER_NAMESPACE::TestGdbPrinters(); }
