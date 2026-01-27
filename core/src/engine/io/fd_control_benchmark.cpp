#include <benchmark/benchmark.h>

#include <unistd.h>

#include <userver/engine/run_standalone.hpp>
#include <utils/check_syscall.hpp>

#include "fd_control.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

class Pipe final {
public:
    Pipe() { utils::CheckSyscall(::pipe(fd_), "creating pipe"); }
    ~Pipe() {
        if (fd_[0] != -1) {
            ::close(fd_[0]);
        }
        if (fd_[1] != -1) {
            ::close(fd_[1]);
        }
    }

    int ExtractIn() { return std::exchange(fd_[0], -1); }
    int ExtractOut() { return std::exchange(fd_[1], -1); }

private:
    int fd_[2]{};
};

namespace io = engine::io;
using Deadline = engine::Deadline;
using FdControl = io::impl::FdControl;

}  // namespace

void FdControlDestroy(benchmark::State& state) {
    engine::RunStandalone([&]() {
        for ([[maybe_unused]] auto _ : state) {
            state.PauseTiming();
            Pipe pipe;

            auto write_control = FdControl::Adopt(pipe.ExtractOut());
            state.ResumeTiming();

            // destructor called here
        }
    });
}
BENCHMARK(FdControlDestroy);

void FdControlCloseDestroy(benchmark::State& state) {
    engine::RunStandalone([&] {
        for ([[maybe_unused]] auto _ : state) {
            state.PauseTiming();
            Pipe pipe;

            auto write_control = FdControl::Adopt(pipe.ExtractOut());
            state.ResumeTiming();

            write_control->Close();
            // destructor called here
        }
    });
}
BENCHMARK(FdControlCloseDestroy);

void FdControlWaitDestroy(benchmark::State& state) {
    engine::RunStandalone([&] {
        for ([[maybe_unused]] auto _ : state) {
            state.PauseTiming();
            Pipe pipe;

            auto write_control = FdControl::Adopt(pipe.ExtractOut());
            auto& write_dir = write_control->Write();
            state.ResumeTiming();

            [[maybe_unused]] auto result = write_dir.Wait(Deadline::Passed());
        }
    });
}
BENCHMARK(FdControlWaitDestroy);

void FdControlConstructWaitDestroy(benchmark::State& state) {
    engine::RunStandalone([&] {
        for ([[maybe_unused]] auto _ : state) {
            state.PauseTiming();
            Pipe pipe;
            state.ResumeTiming();

            auto write_control = FdControl::Adopt(pipe.ExtractOut());
            auto& write_dir = write_control->Write();
            [[maybe_unused]] auto result = write_dir.Wait(Deadline::Passed());
        }
    });
}
BENCHMARK(FdControlConstructWaitDestroy);

USERVER_NAMESPACE_END
