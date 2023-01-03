#include <userver/server/handlers/coro_debug.hpp>

//#include <unwind.h>
#include <libunwind.h>

#include <cstdio>
#include <iostream>
#include <thread>
#include <vector>

#include <userver/components/component_context.hpp>
#include <userver/components/manager.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/utils/thread_name.hpp>

#include <engine/task/task_context.hpp>
#include <engine/task/task_processor.hpp>
#include <engine/task/task_processor_pools.hpp>

#include <boost/stacktrace.hpp>
#include <uboost_coro/coroutine2/detail/push_control_block_cc.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

namespace {

constexpr std::size_t kProcMapsLineMaxLength = PATH_MAX + 100;

struct MMapInfo final {
  std::uintptr_t addr_start;
  std::uintptr_t addr_end;
  std::size_t length;

  char perm[5];

  long offset;

  char dev[12];

  int inode;

  std::string pathname;
};

formats::json::Value Serialize(const MMapInfo& mmap_info,
                               formats::serialize::To<formats::json::Value>) {
  formats::json::ValueBuilder builder{};
  builder["start"] = mmap_info.addr_start;
  builder["end"] = mmap_info.addr_end;
  builder["length"] = mmap_info.length;

  return builder.ExtractValue();
}

void SplitMMapInfoLine(const char* buf, char* addr1, char* addr2, char* perm,
                       char* offset, char* device, char* inode,
                       char* pathname) {
  //
  int orig = 0;
  int i = 0;
  // addr1
  while (buf[i] != '-') {
    addr1[i - orig] = buf[i];
    i++;
  }
  addr1[i] = '\0';
  i++;
  // addr2
  orig = i;
  while (buf[i] != '\t' && buf[i] != ' ') {
    addr2[i - orig] = buf[i];
    i++;
  }
  addr2[i - orig] = '\0';

  // perm
  while (buf[i] == '\t' || buf[i] == ' ') i++;
  orig = i;
  while (buf[i] != '\t' && buf[i] != ' ') {
    perm[i - orig] = buf[i];
    i++;
  }
  perm[i - orig] = '\0';
  // offset
  while (buf[i] == '\t' || buf[i] == ' ') i++;
  orig = i;
  while (buf[i] != '\t' && buf[i] != ' ') {
    offset[i - orig] = buf[i];
    i++;
  }
  offset[i - orig] = '\0';
  // dev
  while (buf[i] == '\t' || buf[i] == ' ') i++;
  orig = i;
  while (buf[i] != '\t' && buf[i] != ' ') {
    device[i - orig] = buf[i];
    i++;
  }
  device[i - orig] = '\0';
  // inode
  while (buf[i] == '\t' || buf[i] == ' ') i++;
  orig = i;
  while (buf[i] != '\t' && buf[i] != ' ') {
    inode[i - orig] = buf[i];
    i++;
  }
  inode[i - orig] = '\0';
  // pathname
  pathname[0] = '\0';
  while (buf[i] == '\t' || buf[i] == ' ') i++;
  orig = i;
  while (buf[i] != '\t' && buf[i] != ' ' && buf[i] != '\n') {
    pathname[i - orig] = buf[i];
    i++;
  }
  pathname[i - orig] = '\0';
}

std::vector<MMapInfo> ParseSelfProcMaps() {
  FILE* f = fopen("/proc/self/maps", "r");
  if (f == nullptr) {
    // TODO : ?
    return {};
  }
  utils::FastScopeGuard close_guard{[f]() noexcept { fclose(f); }};

  char buff[kProcMapsLineMaxLength];
  char addr1[22];
  char addr2[22];
  char perm[8];
  char offset[20];
  char dev[10];
  char inode[30];
  char pathname[PATH_MAX];

  std::vector<MMapInfo> result;

  while (!feof(f)) {
    if (fgets(buff, kProcMapsLineMaxLength, f) == nullptr) {
      break;
    }

    SplitMMapInfoLine(buff, addr1, addr2, perm, offset, dev, inode, pathname);

    MMapInfo mmap_info{};

    char* end = nullptr;
    mmap_info.addr_start = std::strtoul(addr1, &end, 16);
    end = nullptr;
    mmap_info.addr_end = std::strtoul(addr2, &end, 16);
    mmap_info.length = mmap_info.addr_end - mmap_info.addr_start;

    result.push_back(std::move(mmap_info));
  }

  return result;
}

struct BacktraceState {
  const ucontext_t& ucontext;
  size_t address_count = 0;
  static const size_t address_count_max = 30;
  uintptr_t addresses[address_count_max] = {};

  BacktraceState(const ucontext_t& ucontext) : ucontext(ucontext) {}

  bool AddAddress(uintptr_t ip) {
    // No more space in the storage. Fail.
    if (address_count >= address_count_max) return false;

    // Reset the Thumb bit, if it is set.
    const uintptr_t thumb_bit = 1;
    ip &= ~thumb_bit;

    // Ignore null addresses.
    // They sometimes happen when using _Unwind_Backtrace()
    // with the compiler optimizations,
    // when the Link Register is overwritten by the inner
    // stack frames.
    if (ip == 0) return true;

    // Ignore duplicate addresses.
    // They sometimes happen when using _Unwind_Backtrace()
    // with the compiler optimizations,
    // because we both add the second address from the Link Register
    // in ProcessRegisters() and receive the same address
    // in UnwindBacktraceCallback().
    if (address_count > 0 && ip == addresses[address_count - 1]) return true;

    // Finally add the address to the storage.
    addresses[address_count++] = ip;
    return true;
  }
};

void CaptureBacktraceWithLibUnwind(BacktraceState& state) {
  unw_context_t unw_context = {};
  unw_getcontext(&unw_context);
  unw_cursor_t unw_cursor = {};
  unw_init_local(&unw_cursor, &unw_context);

  const auto& ucontext = state.ucontext;
  const auto& gregs = ucontext.uc_mcontext.gregs;

  unw_set_reg(&unw_cursor, UNW_X86_64_RAX, gregs[REG_RAX]);
  unw_set_reg(&unw_cursor, UNW_X86_64_RDX, gregs[REG_RDX]);
  unw_set_reg(&unw_cursor, UNW_X86_64_RCX, gregs[REG_RCX]);
  unw_set_reg(&unw_cursor, UNW_X86_64_RBX, gregs[REG_RBX]);
  unw_set_reg(&unw_cursor, UNW_X86_64_RSI, gregs[REG_RSI]);
  unw_set_reg(&unw_cursor, UNW_X86_64_RDI, gregs[REG_RDI]);
  unw_set_reg(&unw_cursor, UNW_X86_64_RBP, gregs[REG_RBP]);
  unw_set_reg(&unw_cursor, UNW_X86_64_RSP, gregs[REG_RSP]);
  /*unw_set_reg(&unw_cursor, UNW_X86_64_R8, signal_mcontext->arm_r8);
  unw_set_reg(&unw_cursor, UNW_X86_64_R9, signal_mcontext->arm_r9);
  unw_set_reg(&unw_cursor, UNW_X86_64_R10, signal_mcontext->arm_r10);
  unw_set_reg(&unw_cursor, UNW_X86_64_R11, signal_mcontext->arm_fp);
  unw_set_reg(&unw_cursor, UNW_X86_64_R12, signal_mcontext->arm_ip);
  unw_set_reg(&unw_cursor, UNW_X86_64_R13, signal_mcontext->arm_sp);
  unw_set_reg(&unw_cursor, UNW_X86_64_R14, signal_mcontext->arm_lr);
  unw_set_reg(&unw_cursor, UNW_X86_64_R15, signal_mcontext->arm_pc);*/

  unw_set_reg(&unw_cursor, UNW_REG_IP, gregs[REG_RIP]);
  unw_set_reg(&unw_cursor, UNW_REG_SP, gregs[REG_RSP]);

  // unw_step() does not return the first IP.
  state.AddAddress(gregs[REG_RIP]);

  // Unwind frames one by one, going up the frame stack.
  while (unw_step(&unw_cursor) > 0) {
    unw_word_t ip = 0;
    unw_get_reg(&unw_cursor, UNW_REG_IP, &ip);

    bool ok = state.AddAddress(ip);
    if (!ok) break;
  }
}

void UnwindCoro(ucontext_t& context) {
  BacktraceState state{context};
  CaptureBacktraceWithLibUnwind(state);

  int a = 5;

  auto st = boost::stacktrace::stacktrace::from_dump(
      &state.addresses[0], sizeof(void*) * state.address_count);

  std::cout << st << std::endl;

  int b = 5;
}

void CollectStackTrace(const MMapInfo mmap_info) {
  using Coro = engine::coro::Pool<engine::impl::TaskContext>::Coroutine;

  constexpr std::size_t func_alignment = 64;  // alignof( ControlBlock);
  constexpr std::size_t func_size = sizeof(Coro::control_block);

  // reserve space on stack
  void* sp =
      reinterpret_cast<char*>(mmap_info.addr_end) - func_size - func_alignment;
  // align sp pointer
  std::size_t space = func_size + func_alignment;
  sp = std::align(func_alignment, func_size, sp, space);

  auto& control_block = *reinterpret_cast<Coro::control_block*>(sp);
  if (!control_block.valid() || !control_block.c) {
    return;
  }

  auto& context =
      (*reinterpret_cast<boost::context::detail::fiber_activation_record**>(
           &control_block.c))
          ->uctx;

  UnwindCoro(context);

  int a = 5;
}

}  // namespace

CoroDebug::CoroDebug(const components::ComponentConfig& config,
                     const components::ComponentContext& context)
    : HttpHandlerJsonBase{config, context}, manager_{context.GetManager()} {}

CoroDebug::~CoroDebug() = default;

formats::json::Value CoroDebug::HandleRequestJsonThrow(
    const http::HttpRequest&, const formats::json::Value&,
    request::RequestContext&) const {
  auto t = engine::AsyncNoSpan(
      [] { engine::SleepFor(std::chrono::milliseconds{2500}); });
  auto& we = engine::current_task::GetCurrentTaskContext();
  std::lock_guard<std::mutex> lock{processing_mutex_};

  formats::json::ValueBuilder result{};
  do_yield_.store(true);
  std::thread worker{[this, &result] {
    utils::SetCurrentThreadName("coro_debug");
    StopTheWorld();
    CollectDebugInfo(result);
    ResumeTheWorld();
  }};
  while (do_yield_) {
    engine::Yield();
  }

  worker.join();

  t.Wait();

  return result.ExtractValue();
}

void CoroDebug::StopTheWorld() const {
  for (const auto& [k, task_processor] : manager_.GetTaskProcessorsMap()) {
    task_processor->Pause();
  }
  do_yield_.store(false);
}

void CoroDebug::CollectDebugInfo(formats::json::ValueBuilder& result) const {
  const auto mmap_size =
      manager_.GetTaskProcessorPools()->GetCoroPool().GetStackSize();
  // manager_.GetTaskProcessorPools()->GetCoroPool().GetMMapSize();
  auto maps = ParseSelfProcMaps();

  maps.erase(std::remove_if(maps.begin(), maps.end(),
                            [mmap_size](const MMapInfo& mmap_info) {
                              return mmap_info.length != mmap_size;
                            }),
             maps.end());

  for (const auto& mmap_info : maps) {
    CollectStackTrace(mmap_info);
  }

  result["maps"] = maps;
}

void CoroDebug::ResumeTheWorld() const {
  for (const auto& [k, task_processor] : manager_.GetTaskProcessorsMap()) {
    task_processor->Resume();
  }
}

}  // namespace server::handlers

USERVER_NAMESPACE_END
