#!/usr/bin/env bash
# HttpArena-style local load for samples/benchmark_service.
#
#   ./bench.sh --list
#   ./bench.sh --out ~/out --bin ./userver-functional-test-service -- json
#   ./bench.sh --out ~/out --flame --bin ./svc --duration 10s --conns 128 -- json
#
# Flags (only --long). --out is required (no default). Profiles after -- or trailing.
# Env: RUNS DATABASE_URL UPLOAD_SIZE YA YA_PERF FLAMEGRAPH_DIR PERF_EVENT
#      PERF_MAP_TIMEOUT FLAME_MAP_WAIT
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT=
LUA="$ROOT/bench/lua"
DURATION=5s
CORES="$(nproc 2>/dev/null || echo 2)"
THREADS=$((CORES / 2 > 0 ? CORES / 2 : 1))
RUNS="${RUNS:-1}"
HOST=http://127.0.0.1:8080
TLS_HOST=https://localhost:8081
UPLOAD_SIZE="${UPLOAD_SIZE:-65536}"
DO_FLAME=0
KEEP=0
BIN=
PROFILES_RUN=()

die() { echo "[FAIL] $*" >&2; exit 1; }
info() { echo "[info] $*"; }

# name|conns|url|lua
P=(
  "baseline|512|$HOST|$LUA/baseline.lua"
  "pipelined|512|$HOST/pipeline|-"
  "limited-conn|512|$HOST|$LUA/baseline.lua"
  "json|512|$HOST|$LUA/json.lua"
  "json-comp|512|$HOST|$LUA/json_comp.lua"
  "json-tls|512|$TLS_HOST|$LUA/json.lua"
  "upload|128|$HOST|$LUA/upload.lua"
  "static|512|$HOST|$LUA/static.lua"
  "async-db|512|$HOST|$LUA/async_db.lua"
  "api-4|256|$HOST|$LUA/api_mix.lua"
  "api-16|1024|$HOST|$LUA/api_mix.lua"
)
DEFAULT=(baseline pipelined json json-comp upload static)

usage() {
  sed -n '2,12p' "$0" | sed 's/^# \{0,1\}//'
  echo "Profiles: ${P[*]%%|*}"
}

prow() {
  local w=$1 r n
  for r in "${P[@]}"; do
    n=${r%%|*}
    [ "$n" = "$w" ] && { echo "$r"; return; }
  done
  return 1
}

dur_s() { case $1 in *s) echo "${1%s}";; *m) echo $((${1%m}*60));; *) echo "$1";; esac; }

wait_ready() {
  local i
  for i in $(seq 1 50); do
    curl -fsS --max-time 1 "$HOST/pipeline" >/dev/null 2>&1 && return
    sleep 0.1
  done
  return 1
}

find_bin() {
  if [ -n "$BIN" ]; then echo "$BIN"; return; fi
  [ -x ./userver-samples-benchmark_service ] && { echo "$(pwd)/userver-samples-benchmark_service"; return; }
  die "binary './userver-samples-benchmark_service' not found in $(pwd); pass --bin PATH"
}

start_service() {
  local bin=$1
  command -v realpath >/dev/null && bin=$(realpath "$bin") || [[ $bin == /* ]] || bin=$(pwd)/$bin
  [ -x "$bin" ] || die "not executable: $bin"
  info "starting $bin"
  ( cd "$ROOT"; export DATABASE_URL="${DATABASE_URL:-postgresql://testsuite@localhost:15433/postgres}"
    exec "$bin" -c static_config.yaml ) >"$OUT/service.log" 2>&1 &
  SERVICE_PID=$!
  wait_ready || { tail -40 "$OUT/service.log" >&2; die "service not ready ($OUT/service.log)"; }
  info "service pid=$SERVICE_PID"
}

stop_service() {
  [ -n "${SERVICE_PID:-}" ] && kill -0 "$SERVICE_PID" 2>/dev/null || return 0
  info "stopping service pid=$SERVICE_PID"
  kill "$SERVICE_PID" 2>/dev/null || true
  wait "$SERVICE_PID" 2>/dev/null || true
  SERVICE_PID=
}

ya_perf() {
  [ -n "${YA_PERF:-}" ] && [ -x "$YA_PERF" ] && { echo "$YA_PERF"; return; }
  local p ya
  p=$(ls -t "$HOME"/.ya/tools/v4/*/bin/perf 2>/dev/null | head -1 || true)
  if [ -z "$p" ]; then
    ya=${YA:-}
    [ -z "$ya" ] && for ya in "$HOME/workspace/arcadia/ya" "$HOME/workspace/arcadia2/ya" "$HOME/arcadia/ya"; do
      [ -x "$ya" ] && break
    done
    [ -x "${ya:-}" ] || die "ya not found (set YA=); needed to fetch perf"
    info "fetching ya tool perf…"
    "$ya" tool perf --version >/dev/null 2>&1 || true
    p=$(ls -t "$HOME"/.ya/tools/v4/*/bin/perf 2>/dev/null | head -1 || true)
  fi
  [ -x "${p:-}" ] || die "perf not under ~/.ya/tools (ya tool perf --version)"
  echo "$p"
}

flame_dir() {
  [ -n "${FLAMEGRAPH_DIR:-}" ] && { echo "$FLAMEGRAPH_DIR"; return; }
  local d=$ROOT
  while [ "$d" != / ]; do
    [ -f "$d/contrib/tools/flame-graph/flamegraph.pl" ] && { echo "$d/contrib/tools/flame-graph"; return; }
    d=$(dirname "$d")
  done
  for d in "$HOME/workspace/arcadia2" "$HOME/workspace/arcadia" "$HOME/arcadia"; do
    [ -f "$d/contrib/tools/flame-graph/flamegraph.pl" ] && { echo "$d/contrib/tools/flame-graph"; return; }
  done
  die "FlameGraph not found (set FLAMEGRAPH_DIR)"
}

flame_start() {
  local pid=$1 tag=$2
  [ "$DO_FLAME" = 1 ] || return 0
  [ -n "$pid" ] && [ "$pid" != 0 ] || die "--flame needs a live service pid (--bin)"
  PERF_BIN=$(ya_perf)
  FLAME_DATA=$OUT/$tag.perf.data
  rm -f "$FLAME_DATA" "$OUT/$tag.perf" "$OUT/$tag.folded" "$OUT/$tag.flame.svg"
  local map_t=${PERF_MAP_TIMEOUT:-120000} map_w=${FLAME_MAP_WAIT:-8}
  local total=$((map_w + $(dur_s "$DURATION") + 5))
  info "perf record -e ${PERF_EVENT:-cycles} → $FLAME_DATA (pid=$pid, ${total}s)"
  "$PERF_BIN" record -e "${PERF_EVENT:-cycles}" -F 250 --call-graph dwarf -g \
    -p "$pid" --proc-map-timeout="$map_t" -o "$FLAME_DATA" -- /bin/sleep "$total" \
    >"$OUT/$tag.perf.log" 2>&1 &
  FLAME_PID=$!
  sleep 0.5
  kill -0 "$FLAME_PID" 2>/dev/null || { cat "$OUT/$tag.perf.log" >&2; die "perf record failed"; }
  info "waiting ${map_w}s for /proc maps…"
  sleep "$map_w"
  kill -0 "$FLAME_PID" 2>/dev/null || { cat "$OUT/$tag.perf.log" >&2; die "perf died during maps wait"; }
}

flame_finish() {
  local tag=$1 n fg
  [ "$DO_FLAME" = 1 ] && [ -n "${FLAME_PID:-}" ] || return 0
  info "waiting for perf…"
  wait "$FLAME_PID" 2>/dev/null || true
  FLAME_PID=
  [ -f "$FLAME_DATA" ] || return 0
  n=$(grep -oE '\([0-9]+ samples\)' "$OUT/$tag.perf.log" 2>/dev/null | tail -1 | grep -oE '[0-9]+' || true)
  [ -n "$n" ] && [ "$n" -gt 0 ] || { cat "$OUT/$tag.perf.log" >&2; die "no samples"; }
  info "perf: $n samples → script/fold/svg"
  "$(ya_perf)" script -i "$FLAME_DATA" >"$OUT/$tag.perf"
  [ -s "$OUT/$tag.perf" ] || die "empty perf script"
  # main-worker_0 / main-worker:21 → main-worker
  sed -i -E '/^[ \t]/b; s/^([A-Za-z0-9_-]+)[_:][0-9]+/\1/' "$OUT/$tag.perf"
  fg=$(flame_dir)
  if command -v c++filt >/dev/null; then
    "$fg/stackcollapse-perf.pl" "$OUT/$tag.perf" | c++filt >"$OUT/$tag.folded"
  else
    "$fg/stackcollapse-perf.pl" "$OUT/$tag.perf" >"$OUT/$tag.folded"
  fi
  [ -s "$OUT/$tag.folded" ] || die "empty folded stacks"
  "$fg/flamegraph.pl" "$OUT/$tag.folded" >"$OUT/$tag.flame.svg"
  [ -s "$OUT/$tag.flame.svg" ] || die "empty svg"
  info "SVG $OUT/$tag.flame.svg ($(wc -c <"$OUT/$tag.flame.svg") bytes)"
}

run_one() {
  local name=$1 row conns url script out rps best=-1 best_out= run pid=
  row=$(prow "$name") || die "unknown profile: $name (see --list)"
  IFS='|' read -r _ conns url script <<<"$row"
  conns=${CONNS:-$conns}
  if [ "$name" = upload ]; then
    local body=$OUT/upload.body
    [ -f "$body" ] && [ "$(wc -c <"$body")" -eq "$UPLOAD_SIZE" ] || head -c "$UPLOAD_SIZE" /dev/urandom >"$body"
    export BENCH_UPLOAD_BODY=$body
  fi
  echo; echo "=== $name  -c $conns -t $THREADS -d $DURATION  ($RUNS run(s)) ==="
  pid=${SERVICE_PID:-}
  [ -n "$pid" ] || pid=$(pgrep -n -f 'userver-samples-benchmark_service|userver_httparena|userver-functional-test-service' 2>/dev/null | head -1 || true)
  for run in $(seq 1 "$RUNS"); do
    echo "── run $run/$RUNS"
    [ "$run" = 1 ] && flame_start "${pid:-0}" "$name"
    local -a cmd=(wrk -t "$THREADS" -c "$conns" -d "$DURATION" --latency)
    [ "$script" != - ] && cmd+=(-s "$script")
    cmd+=("$url")
    info "+ ${cmd[*]}"
    out=$("${cmd[@]}" 2>&1 || true)
    echo "$out" | tail -20
    rps=$(echo "$out" | grep -oE 'Requests/sec:[[:space:]]+[0-9.]+' | head -1 | awk '{printf "%d\n",$2}')
    rps=${rps:-0}
    [ "$rps" -gt "$best" ] && { best=$rps; best_out=$out; }
    [ "$run" = 1 ] && flame_finish "$name"
    [ "$run" -lt "$RUNS" ] && sleep 1
  done
  printf '%s\n' "$best_out" >"$OUT/$name.wrk.txt"
  echo "$best" >"$OUT/$name.rps"
  info "best RPS: $best"
}

# ── argv: only --long flags; profiles after -- or trailing ───────────────
while [ $# -gt 0 ]; do
  case $1 in
    --list) for r in "${P[@]}"; do printf '  %-14s -c %s\n' "${r%%|*}" "$(echo "$r"|cut -d\| -f2)"; done
            echo "default: ${DEFAULT[*]}"; exit 0 ;;
    --help) usage; exit 0 ;;
    --flame) DO_FLAME=1 ;;
    --keep) KEEP=1 ;;
    --out) OUT=$2; shift ;;
    --bin) BIN=$2; shift ;;
    --host) HOST=$2; shift ;;
    --conns) CONNS=$2; shift ;;
    --threads) THREADS=$2; shift ;;
    --duration) DURATION=$2; shift ;;
    --runs) RUNS=$2; shift ;;
    --) shift; PROFILES_RUN+=("$@"); break ;;
    --*) die "unknown flag: $1 (see --help)" ;;
    *) PROFILES_RUN+=("$1") ;;
  esac
  shift
done

command -v wrk >/dev/null || die "need wrk"
command -v curl >/dev/null || die "need curl"
[ -n "$OUT" ] || die "required: --out DIR (e.g. --out ~/out)"
[[ $OUT == ~* ]] && OUT="${OUT/#\~/$HOME}"
mkdir -p "$OUT"

SERVICE_PID=
trap '[ -n "${FLAME_PID:-}" ] && kill -INT "$FLAME_PID" 2>/dev/null || true; [ "$KEEP" = 1 ] || stop_service' EXIT

if wait_ready; then
  info "using service at $HOST"
else
  start_service "$(find_bin)"
fi

[ ${#PROFILES_RUN[@]} -eq 0 ] && PROFILES_RUN=("${DEFAULT[@]}")
sum=()
for p in "${PROFILES_RUN[@]}"; do
  run_one "$p"
  sum+=("$p:$(cat "$OUT/$p.rps")")
done
echo; echo "=== summary ==="
printf '  %s\n' "${sum[@]}"
info "artifacts in $OUT"
