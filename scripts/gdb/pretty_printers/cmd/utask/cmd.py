import argparse
from collections.abc import Iterator
from contextlib import contextmanager
from dataclasses import dataclass
import functools
import itertools
import re
import traceback
import warnings

import gdb
from gdb.FrameDecorator import FrameDecorator as gdb_FrameDecorator
import gdb.unwinder

if not hasattr(gdb.unwinder, 'FrameId'):

    @dataclass
    class FrameId:
        pc: gdb.Value | int
        sp: gdb.Value | int
        special: gdb.Value | int | None = None

    gdb.unwinder.FrameId = FrameId


arch_name: str = re.search(
    'boost.*context.*/asm/jump_([a-z_0-9]+)',
    gdb.execute('info sources boost.*context.*/asm/jump_', to_string=True),
).group(1)
registers_offsets = {
    'x86_64_sysv_elf_gas': {
        'r12': 0x10,
        'r13': 0x18,
        'r14': 0x20,
        'r15': 0x28,
        'rbx': 0x30,
        'rbp': 0x38,
        'pc': 0x40,
        'sp': 0x48,
    },
}[arch_name]

USERVER_NAMESPACE = (
    match := re.search(
        '([0-9a-zA-Z_:]*)impl::VerySpecialUniqueClassForUserverNamespaceDetection<void>',
        gdb.execute('info types -q impl::VerySpecialUniqueClassForUserverNamespaceDetection<void>', to_string=True),
    )
) and match.group(1)
if USERVER_NAMESPACE is None:
    warnings.warn('Failed to detect USERVER_NAMESPACE')
    USERVER_NAMESPACE = ''


class FrameUnwinder(gdb.unwinder.Unwinder):
    """
    Called exactly once per an unwinding process.
    First call creates a synthetic frame and saves context registers into it.
    Subsequent calls do nothing.
    As a result, second (#1) frame and subsequent ones are replaced with context frames
    """

    def __init__(self, name):
        super(FrameUnwinder, self).__init__(name)
        self.ctx_registers: dict[str, gdb.Value] = {}
        self.enabled = False
        self.was_called = False

    def __call__(self, pending_frame: gdb.PendingFrame):
        if not self.enabled or not self.ctx_registers or self.was_called:
            return None
        self.was_called = True

        unwind_info = pending_frame.create_unwind_info(gdb.unwinder.FrameId(gdb.Value(0), gdb.Value(0)))
        for reg_name, value in self.ctx_registers.items():
            unwind_info.add_saved_register(reg_name, value)

        return unwind_info

    def setup(self, regs):
        self.ctx_registers = regs
        self.was_called = False

    def enable(self):
        self.enabled = True

    def disable(self):
        self.enabled = False

    @contextmanager
    def get_target_frame(self):
        try:
            frame = gdb.selected_frame()
            if not (frame and frame.is_valid() and frame.pc() == self.ctx_registers['pc']):
                self.enable()
                if gdb.convenience_variable('_gdb_major') >= 14:
                    # Unwind from synthetic frame
                    gdb.execute('select-frame view 0x1 0')
                    frame = gdb.selected_frame()
                else:
                    # `select-frame view` is quite unstable in gdb < 14.0
                    # so just unwind from the newest frame instead.
                    # frame levels in backtrace may be shifted
                    gdb.invalidate_cached_frames()
                    frame = gdb.newest_frame()
                while frame and frame.pc() != self.ctx_registers['pc']:
                    frame = frame.older()
            yield frame
        finally:
            self.disable()


context_switch_unwinder = FrameUnwinder('userver_context_switch')
gdb.unwinder.register_unwinder(None, context_switch_unwinder, True)


class FrameFilter:
    """Creates the context backtrace (via enabled unwinder) rather than filters frames"""

    def __init__(self, name: str):
        self.name = name
        self.priority = 100
        self.enabled = False

    def filter(self, _: Iterator[gdb_FrameDecorator]) -> Iterator[gdb_FrameDecorator]:
        with context_switch_unwinder.get_target_frame() as frame:
            while frame:
                yield gdb_FrameDecorator(frame)
                frame = frame.older()

    def setup(self):
        self.enabled = True

    def disable(self):
        self.enabled = False


context_switch_filter = FrameFilter('userver_context_switch')
gdb.frame_filters[context_switch_filter.name] = context_switch_filter


void_pp_t = gdb.lookup_type('void').pointer().pointer()


class SafeFrame:
    """Generalization of gdb.Frame in case of its invalidation"""

    def __init__(self, frame: gdb.Frame, level: int):
        self._frame = frame
        self._level = level

    @staticmethod
    def selected_frame():
        frame = gdb.selected_frame()
        return SafeFrame(frame, frame.level())

    def select(self):
        if self._frame.is_valid():
            self._frame.select()
        else:
            gdb.execute(f'select-frame {self._level}', to_string=True)


class BoostFContext:
    def __init__(self, ctx: gdb.Value):
        self.ctx = ctx

    def read_registers(self):
        result: dict[str, gdb.Value] = {}
        for reg_name, reg_offset in registers_offsets.items():
            reg_value = self.ctx + reg_offset
            if reg_name != 'sp':
                reg_value = reg_value.cast(void_pp_t).dereference()
            result[reg_name] = reg_value
        return result

    @contextmanager
    def switch_to(self):
        # finds target frame (the newest one of saved context) using unwinder and switches to it
        old_frame = SafeFrame.selected_frame()
        registers = self.read_registers()

        try:
            context_switch_unwinder.setup(registers)
            with context_switch_unwinder.get_target_frame() as frame:
                if frame:
                    context_switch_filter.setup()  # needed only for backtrace command
                    frame.select()
                    yield frame
                else:
                    yield None
        finally:
            context_switch_filter.disable()
            old_frame.select()
            gdb.invalidate_cached_frames()


def unwrap_optional(value: gdb.Value) -> gdb.Value | None:
    for _, val in gdb.default_visualizer(value).children():
        return val
    return None


def get_unique_ptr(value: gdb.Value) -> gdb.Value:
    for _, ptr in gdb.default_visualizer(value).children():
        return ptr
    raise ValueError('Failed to get unique_ptr')


class TaskContext:
    def __init__(self, task: gdb.Value, thread: gdb.InferiorThread | None = None):
        self.task = task
        self.thread = thread
        self.task_id = hex(int(task))

    @functools.cached_property
    def state(self):
        atomic_state = self.task['state_']
        return self._state_enum_to_str(str(atomic_state.cast(atomic_state.type.template_argument(0))))

    @functools.cached_property
    def _fctx(self):
        return (coro := unwrap_optional(self.task['coro_']['coro_'])) and coro['coro_']['cb_']['c']['fctx_']

    @contextmanager
    def switch_to(self):
        if fctx := self._fctx:
            with BoostFContext(fctx).switch_to() as frame:
                if frame is not None:
                    yield
                else:
                    raise RuntimeError('Failed to switch to task context')
        elif self.thread:
            old_thread = gdb.selected_thread()
            try:
                self.thread.switch()
                yield
            finally:
                old_thread.switch()
        else:
            raise RuntimeError('Task is not switchable')

    @functools.cached_property
    def attached_span(self):
        return Span.from_task(self.task)

    @functools.cache
    @staticmethod
    def possible_states():
        try:
            return [
                TaskContext._state_enum_to_str(match.group(1))
                for match in re.finditer(
                    re.escape(f'{USERVER_NAMESPACE}engine::TaskBase::State::') + '([a-zA-Z0-9_]+)',
                    gdb.execute(f"ptype '{USERVER_NAMESPACE}engine::TaskBase::State'", to_string=True),
                )
            ]
        except gdb.error:
            return ['invalid', 'new', 'queued', 'running', 'suspended', 'cancelled', 'completed']

    @staticmethod
    def _state_enum_to_str(state: str):
        return state.split('::')[-1][1:].lower()

    @functools.cache
    @staticmethod
    def _native_type():
        return gdb.lookup_type(f'{USERVER_NAMESPACE}engine::impl::TaskContext')


class Span:
    def __init__(self, span: gdb.Value):
        self.span = span

    @staticmethod
    def from_task(task: gdb.Value):
        local_storage = unwrap_optional(task['local_storage_'])
        if not local_storage:
            return None
        storage_impl = local_storage['impl_']['storage_'].address.cast(Span._storage_impl_type().pointer())
        data_ptr = get_unique_ptr(storage_impl['data'])
        if not data_ptr:
            return None
        task_local_spans = gdb.lookup_static_symbol(
            f'{USERVER_NAMESPACE}tracing::(anonymous namespace)::task_local_spans',
        ).value()
        data_key = task_local_spans['impl_']['key_']
        data_base = (data_ptr + data_key)['ptr']
        if not data_base:
            return None
        data_impl = data_base.cast(Span._data_impl_type().pointer())
        span_list_root = data_impl['variable_']['data_']['root_plus_size_']['m_header']
        if not span_list_root or int(span_list_root.address) == int(span_list_root['next_']):
            return None
        raw_span = span_list_root['prev_']
        return Span(gdb.parse_and_eval(f'({Span._span_impl_type().name}*){raw_span}'))

    @functools.cached_property
    def name(self) -> str:
        # for some reason, dereferencing of span may be too slow (~10 ms)
        # so, there is just `self.span['name_']` but 4 times faster
        name_field = next(field for field in self.span.type.target().fields() if field.name == 'name_')
        name_ptr = gdb.Value(int(self.span) + (name_field.bitpos // 8)).cast(name_field.type.pointer())
        return str(name_ptr.dereference())[1:-1]

    @functools.cache
    @staticmethod
    def _storage_impl_type():
        return gdb.lookup_type(f'{USERVER_NAMESPACE}engine::impl::task_local::Storage::Impl')

    @functools.cache
    @staticmethod
    def _data_impl_type():
        template = (
            f'{USERVER_NAMESPACE}engine::impl::task_local::DataImpl<'
            f'  boost::intrusive::list<'
            f'    {USERVER_NAMESPACE}tracing::Span::Impl,'
            f'    boost::intrusive::constant_time_size<false>'
            f'  >,'
            '  {kNormal}'
            '>'
        )
        try:
            return gdb.lookup_type(
                template.format(kNormal=f'({USERVER_NAMESPACE}engine::impl::task_local::VariableKind)0'),
            )
        except gdb.error:
            return gdb.lookup_type(
                template.format(kNormal=f'{USERVER_NAMESPACE}engine::impl::task_local::VariableKind::kNormal'),
            )

    @functools.cache
    @staticmethod
    def _span_impl_type():
        return gdb.lookup_type(f'{USERVER_NAMESPACE}tracing::Span::Impl')


def get_task_from_stacktrace() -> gdb.Value | None:
    old_frame = SafeFrame.selected_frame()
    try:
        frame = gdb.selected_frame()
        while frame is not None:
            if (name := frame.name()) and name.startswith(f'{USERVER_NAMESPACE}engine::impl::TaskContext::CoroFunc'):
                frame.select()
                try:
                    if (
                        task := frame.read_var('context', frame.block())
                    ) and task.type == TaskContext._native_type().pointer():
                        return task
                except ValueError:
                    pass
            frame = frame.older()
        return None
    finally:
        old_frame.select()


def get_task_from_control_block(raw_cb_ptr: int):
    if fctx := gdb.Value(raw_cb_ptr).cast(void_pp_t).dereference():
        with BoostFContext(fctx).switch_to() as frame:
            if frame is not None:
                return get_task_from_stacktrace()


def lookup_mappings():
    # sizeof(boost::coroutines2::detail::push_coroutine<T>::control_block) == 32
    CB_SIZE = (
        64  # 64-aligned sizeof(control_block). See final value of `space` in coroutine2/detail/create_control_block.ipp
    )
    PAGE_SIZE = gdb.parse_and_eval(f'{USERVER_NAMESPACE}engine::coro::debug::page_size')
    ALLOC_STACK_SIZE = gdb.parse_and_eval(f'{USERVER_NAMESPACE}engine::coro::debug::allocator_stack_size')
    TAIL_STACK_SIZE = 512  # just big enough to find allocator mark

    is_coredump = (connection := gdb.selected_inferior().connection) and connection.type == 'core'
    hex_re = r'0x[0-9A-Fa-f]+'
    if is_coredump:
        command = 'maintenance info sections READONLY'
        regexp = re.compile(
            rf'\s+\[[0-9]+\]\s+({hex_re})->({hex_re})\s+at\s+{hex_re}:\s+load[0-9]+\s+ALLOC\s+LOAD\s+READONLY\s+HAS_CONTENTS\s*$',
        )
    else:
        command = 'info proc mappings'
        regexp = re.compile(rf'\s*({hex_re})\s+({hex_re})\s+{hex(PAGE_SIZE)}\s+{hex_re}\s+---p\s*$')

    mappings = gdb.execute(command, to_string=True).split('\n')
    for line in mappings:
        if match := regexp.match(line):
            guard_page_begin = int(match.group(1), 16)
            guard_page_end = int(match.group(2), 16)
            if guard_page_end - guard_page_begin != PAGE_SIZE:
                continue
            stack_end = guard_page_end + ALLOC_STACK_SIZE
            try:
                if b'ThisIsCoroAlloc\0' in bytes(
                    gdb.selected_inferior().read_memory(stack_end - TAIL_STACK_SIZE, TAIL_STACK_SIZE),
                ):
                    cb_ptr = stack_end - CB_SIZE
                    if task := get_task_from_control_block(cb_ptr):
                        yield task
            except gdb.MemoryError:
                continue


def get_all_suspended_tasks():
    for task in lookup_mappings():
        yield TaskContext(task)


def get_all_running_tasks():
    # Even though contexts of running tasks are also obtained via lookup_mappings,
    # they are not switchable (it is impossible to switch to a running coroutine).
    # So, they will be just wrappers of threads with access to TaskContext object.
    old_thread = gdb.selected_thread()
    old_frame = SafeFrame.selected_frame()
    active_tasks: list[TaskContext] = []
    try:
        for thread in gdb.selected_inferior().threads():
            thread.switch()
            gdb.newest_frame().select()
            if task := get_task_from_stacktrace():
                active_tasks.append(TaskContext(task, thread))
    finally:
        old_thread.switch()
        old_frame.select()
    for task in active_tasks:
        yield task


def get_all_tasks() -> Iterator[TaskContext]:
    # get from cache (by all registers hash) of find from scratch
    frame = gdb.selected_frame()
    state_hash = hash(tuple(int(frame.read_register(reg)) for reg in frame.architecture().registers('general')))
    successfully_cached = getattr(get_all_tasks, 'successfully_cached', False)
    if state_hash == getattr(get_all_tasks, 'cached_state_hash', None) and successfully_cached:
        for task in getattr(get_all_tasks, 'cached_tasks', list[TaskContext]()):
            yield task
    else:
        get_all_tasks.cached_state_hash = state_hash
        get_all_tasks.cached_tasks = list[TaskContext]()
        for task in itertools.chain(get_all_suspended_tasks(), get_all_running_tasks()):
            get_all_tasks.cached_tasks.append(task)
            yield task
        get_all_tasks.successfully_cached = True


class UtaskCmd(gdb.Command):
    """Examine the context of userver tasks."""

    def __init__(self):
        super().__init__('utask', gdb.COMMAND_STACK, prefix=True)


class UtaskListCmd(gdb.Command):
    def __init__(self):
        super().__init__('utask list', gdb.COMMAND_STATUS)

    @functools.cache
    @staticmethod
    def get_argparser():
        def _regexp_parser(pattern: str):
            try:
                return re.compile(pattern)
            except re.error as e:
                raise argparse.ArgumentTypeError(e)

        parser = argparse.ArgumentParser(
            'utask list',
            description='List userver tasks (all or some of them)',
            exit_on_error=False,
        )
        parser.add_argument(
            '-s',
            '--states',
            action='extend',
            nargs='*',
            metavar='STATE',
            choices=TaskContext.possible_states(),
            help=f'List utasks with specific states only (one of {{{str(TaskContext.possible_states())[1:-1]}}})',
        )
        parser.add_argument('-i', '--id', help='List utask with specific id only')
        parser.add_argument('-n', '--name', type=_regexp_parser, help='List utasks which names match the regex')
        parser.add_argument(
            '-b',
            '--backtrace',
            type=_regexp_parser,
            help='List utasks which backtraces match the regex',
        )
        return parser

    def invoke(self, arg: str, from_tty: bool):
        parser = UtaskListCmd.get_argparser()
        try:
            args = parser.parse_args(gdb.string_to_argv(arg))
        except SystemExit:
            return

        def filterer(task: TaskContext):
            if args.states and task.state not in args.states:
                return False
            if args.id and task.task_id != args.id:
                return False
            if args.name and not args.name.search(task.attached_span.name):
                return False
            if args.backtrace:
                with task.switch_to():
                    backtrace = gdb.execute('bt', to_string=True)
                    if not args.backtrace.search(backtrace):
                        return False
            return True

        try:
            print(f'{"Task ID":14}', f'{"State":9}', 'Span name')
            for task in get_all_tasks():
                if filterer(task):
                    print(f'{task.task_id:14}', f'{task.state:9}', span.name if (span := task.attached_span) else '')
        except Exception:
            traceback.print_exc()
            raise


UtaskListCmd.__doc__ = UtaskListCmd.get_argparser().format_help()


class UtaskApplyCmd(gdb.Command):
    """Execute a gdb command in the userver task context.

    Usage: (gdb) utask apply <utask_id|span_name|"all"> <gdbcmd>

    Choose task by utask_id (hex number, see utask list) or its span name
    Use "all" to apply <gdbcmd> to all tasks.

    For example: `(gdb) utask apply all backtrace`

    During command execution the convenience variables are available:
        '$this_utask' holds pointer to current TaskContext
        '$this_utask_name' holds the name of this task (span name)
    """

    def __init__(self):
        super().__init__('utask apply', gdb.COMMAND_STACK)

    def invoke(self, arg: str, from_tty: bool):
        try:
            args = arg.split(maxsplit=1)
            if len(args) < 2:
                print('Usage: utask apply <utask_id|span_name|"all"> <gdbcmd>')
                return None
            which_task, cmd = args[0], args[1]

            if which_task == 'all':
                task_cnt = 0
                for task in get_all_tasks():
                    task_cnt += 1
                    self.invoke_per_task(task, cmd, from_tty)
                if task_cnt == 0:
                    print('No tasks found')
            else:
                fast_chached_tasks: list[TaskContext] = []
                for task in get_all_tasks():
                    # fast filtering
                    if which_task == task.task_id:
                        return self.invoke_per_task(task, cmd, from_tty)
                    fast_chached_tasks.append(task)
                for task in fast_chached_tasks:
                    # getting attached_span is a bit longer
                    if (span := task.attached_span) and which_task == span.name:
                        return self.invoke_per_task(task, cmd, from_tty)
                raise gdb.error(f'Task "{which_task}" not found')
        except gdb.error:
            traceback.print_exc()
            raise

    def invoke_per_task(self, task: TaskContext, cmd: str, from_tty: bool):
        print(f'Executing command `{cmd}` for task {task.task_id}')
        with task.switch_to():
            try:
                gdb.set_convenience_variable('this_utask', task.task)
                gdb.set_convenience_variable(
                    'this_utask_name', ((task.attached_span and task.attached_span.name) or '')
                )
                gdb.execute(cmd, from_tty)
            finally:
                gdb.set_convenience_variable('this_utask', None)
                gdb.set_convenience_variable('this_utask_name', None)

    def complete(self, text: str, word: str) -> list[str] | int | None:
        complete_args_count = len(text.split()) - int(bool(word))
        match complete_args_count:
            case 0:  # utask apply
                return ['all'] + [
                    task.attached_span.name if task.attached_span else task.task_id
                    for task in get_all_tasks()
                    if not word
                    or (task.attached_span and task.attached_span.name.startswith(word))
                    or task.task_id.startswith(word)
                ]
            case 1:  # utask apply <task_id>
                return gdb.COMPLETE_COMMAND
            case _:
                return None


if __name__ == '__main__':
    print('Registering Utask cmd...')
    try:
        UtaskCmd()
        UtaskListCmd()
        UtaskApplyCmd()
        print('Utask cmd registered')
    except Exception:
        traceback.print_exc()
