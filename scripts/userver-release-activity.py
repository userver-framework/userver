#!/usr/bin/env python3
"""Build Userver Release Activity chart (SVG).

Usage:
    python3 scripts/userver-release-activity.py

Methodology:
- Releases: GitHub userver-framework/userver published_at, tags >= v2.1.
- Features: every leaf changelog bullet except recap sections whose title
  starts with "big new features since the " and items that start with
  "Migration:". Fold vX.Y-rc into vX.Y.
- External PRs: changelog items that thank a GitHub owner; +1 per owner.
- Commits: `arc log --format={date_rfc}` on taxi/uservices/userver in
  (prev published_at, this published_at]. Latest tag also gets later commits.
- One chart, calendar X. Year lines on Jan 1; quarter lines on Apr/Jul/Oct 1.
  Quarterly increment: linear interpolation of each cumulative series at
  quarter start and end (0 before the first release, last value after the
  last). Y = value(end) - value(start). Left axis: features and PRs. Right
  axis: commits.
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
import urllib.request
from collections import defaultdict
from datetime import date, datetime, timezone
from pathlib import Path

GITHUB_API = "https://api.github.com/repos/userver-framework/userver/releases?per_page=100"
MIN_TAG = (2, 1)
CHANGELOG_REL = Path("scripts/docs/en/userver/roadmap_and_changelog.md")
SVG_NAME = "Userver-Release-Activity.svg"

SKIP_SECTION_PREFIX = "big new features since the "
MIGRATION_ITEM_RE = re.compile(r"^migration\s*:", re.I)
HEADER_RE = re.compile(r"^###\s+(.+?)\s*$", re.MULTILINE)
RELEASE_ID_RE = re.compile(r"v\d+(?:\.\d+)+(?:-rc)?", re.IGNORECASE)
GITHUB_USER_RE = re.compile(r"https://github\.com/([A-Za-z0-9_.-]+)")


def userver_root() -> Path:
    here = Path(__file__).resolve()
    if here.parent.name == "scripts" and (here.parent.parent / CHANGELOG_REL).is_file():
        return here.parent.parent
    for p in here.parents:
        if (p / CHANGELOG_REL).is_file():
            return p
    raise SystemExit("Could not find userver root from " + str(here))


def parse_tag(tag: str) -> tuple[int, ...]:
    m = re.match(r"v?(\d+(?:\.\d+)*)", tag)
    if not m:
        return (0,)
    return tuple(int(x) for x in m.group(1).split("."))


def fold_rc(tag: str) -> str:
    return re.sub(r"-rc$", "", tag, flags=re.IGNORECASE)


def parse_iso(ts: str) -> datetime:
    return datetime.fromisoformat(ts.replace("Z", "+00:00"))


def fetch_github_releases() -> list[dict]:
    req = urllib.request.Request(GITHUB_API, headers={"Accept": "application/vnd.github+json"})
    with urllib.request.urlopen(req, timeout=30) as resp:
        rows = json.loads(resp.read().decode())
    out = []
    for row in rows:
        if row.get("draft") or row.get("prerelease"):
            continue
        tag = row["tag_name"]
        if parse_tag(tag) < MIN_TAG:
            continue
        out.append({"tag": tag, "published_at": row["published_at"]})
    out.sort(key=lambda r: parse_iso(r["published_at"]))
    return out


def parse_changelog(path: Path) -> dict[str, dict[str, int]]:
    text = path.read_text(encoding="utf-8")
    idx = text.find("## Changelog")
    if idx < 0:
        raise SystemExit("## Changelog not found")
    text = text[idx:]
    stats: dict[str, dict[str, int]] = defaultdict(lambda: {"features": 0, "external_prs": 0})

    def is_category_header(item: str, has_children: bool) -> bool:
        if not has_children:
            return False
        stripped = re.sub(r"[*`]", "", item.strip().rstrip(":").strip())
        return len(stripped) <= 40 and not re.search(r"[.!?]", stripped)

    def skip_section_name(name: str) -> bool:
        return name.startswith(SKIP_SECTION_PREFIX)

    for part in re.split(r"(?m)^(?=### )", text):
        header_m = HEADER_RE.match(part)
        if not header_m:
            continue
        title = header_m.group(1)
        if title.lower().startswith("older releases"):
            continue
        rid_m = RELEASE_ID_RE.search(title)
        if not rid_m:
            continue
        target = fold_rc(rid_m.group(0).lower())
        if parse_tag(target) < MIN_TAG:
            continue
        body = part.split("\n", 1)[1] if "\n" in part else ""

        collected: list[tuple[str, int, str]] = []
        in_fence = False
        pending: tuple[int, list[str]] | None = None

        def flush() -> None:
            nonlocal pending
            if pending is None:
                return
            collected.append(("bullet", pending[0], " ".join(pending[1]).strip()))
            pending = None

        for raw in body.splitlines():
            if raw.startswith("```"):
                in_fence = not in_fence
                if pending is not None:
                    pending[1].append(raw)
                continue
            if in_fence:
                if pending is not None:
                    pending[1].append(raw)
                continue
            sec = re.match(r"^([A-Za-z][^:\n]{0,60}):\s*$", raw)
            if sec:
                flush()
                collected.append(("section", 0, sec.group(1).strip().lower()))
                continue
            m = re.match(r"^(\s*)\*\s+(.*)$", raw)
            if m:
                flush()
                pending = (len(m.group(1)), [m.group(2)])
                continue
            if pending is not None and raw.strip():
                pending[1].append(raw.strip())
        flush()

        bullet_idxs = [i for i, (kind, _, _) in enumerate(collected) if kind == "bullet"]
        has_child = [False] * len(collected)
        for j, i in enumerate(bullet_idxs):
            if j + 1 < len(bullet_idxs) and collected[bullet_idxs[j + 1]][1] > collected[i][1]:
                has_child[i] = True

        section = ""
        skip_section = False
        for i, (kind, indent, item) in enumerate(collected):
            if kind == "section":
                section = item
                skip_section = skip_section_name(section)
                continue
            if has_child[i] and is_category_header(item, True):
                label = item.strip().rstrip(":").lower()
                if indent == 0:
                    section = label
                    skip_section = skip_section_name(section)
                continue
            if re.search(r"thanks", item, re.I) and "github.com/" in item.lower():
                stats[target]["external_prs"] += max(1, len(GITHUB_USER_RE.findall(item)))
            if skip_section or MIGRATION_ITEM_RE.match(item.lstrip()):
                continue
            stats[target]["features"] += 1
    return stats


def parse_arc_date(line: str) -> datetime | None:
    line = line.strip()
    if not line or line.startswith("WARNING") or line.startswith("Remove") or line.startswith("You can"):
        return None
    normalized = line.replace(" MSK", " +0300").replace(" UTC", " +0000").replace(" GMT", " +0000")
    try:
        return datetime.strptime(normalized, "%a, %d %b %Y %H:%M:%S %z")
    except ValueError:
        return None


def arc_log_dates(repo: Path, after: datetime, before: datetime | None) -> list[datetime]:
    cmd = [
        "arc",
        "log",
        "--after",
        after.strftime("%Y-%m-%dT%H:%M:%S"),
        "--format={date_rfc}",
        "--",
        ".",
    ]
    if before is not None:
        cmd[3:3] = ["--before", before.strftime("%Y-%m-%dT%H:%M:%S")]
    proc = subprocess.run(cmd, cwd=repo, capture_output=True, text=True, check=False)
    if proc.returncode != 0:
        print(proc.stderr, file=sys.stderr)
        raise SystemExit(f"arc log failed: {proc.returncode}")
    dates = []
    for line in proc.stdout.splitlines():
        dt = parse_arc_date(line)
        if dt is not None:
            dates.append(dt.astimezone(timezone.utc))
    return dates


def fill_commits(repo: Path, releases: list[dict]) -> None:
    after = datetime(2024, 5, 15, tzinfo=timezone.utc)
    dates = arc_log_dates(repo, after, None)
    bounds = []
    for i, rel in enumerate(releases):
        prev = parse_iso(releases[i - 1]["published_at"]) if i else datetime(1970, 1, 1, tzinfo=timezone.utc)
        bounds.append((i, prev, parse_iso(rel["published_at"])))
    last = len(releases) - 1
    counts = [0] * len(releases)
    for dt in dates:
        placed = False
        for i, prev, published in bounds:
            if prev < dt <= published:
                counts[i] += 1
                placed = True
                break
        if not placed and dt > bounds[-1][2]:
            counts[last] += 1
    for i, rel in enumerate(releases):
        rel["commits"] = counts[i]


def window_days(releases: list[dict]) -> None:
    for i, rel in enumerate(releases):
        cur = parse_iso(rel["published_at"]).date()
        if i:
            prev = parse_iso(releases[i - 1]["published_at"]).date()
        else:
            prev = date(2024, 5, 15)  # v2.0
        rel["window_days"] = (cur - prev).days
        rel["published"] = cur.isoformat()


def add_totals(releases: list[dict]) -> None:
    feat = prs = commits = 0
    for rel in releases:
        feat += rel["features"]
        prs += rel["external_prs"]
        commits += rel["commits"]
        rel["total_features"] = feat
        rel["total_external_prs"] = prs
        rel["total_commits"] = commits


def quarter_start(d: date) -> date:
    return date(d.year, ((d.month - 1) // 3) * 3 + 1, 1)


def add_months(d: date, months: int) -> date:
    m = d.month - 1 + months
    return date(d.year + m // 12, m % 12 + 1, 1)


def at_utc_midnight(d: date) -> datetime:
    return datetime(d.year, d.month, d.day, tzinfo=timezone.utc)


def interp_cumulative(releases: list[dict], key: str, t: datetime) -> float:
    """Linear interpolation of a cumulative series between release timestamps."""
    times = [parse_iso(r["published_at"]) for r in releases]
    values = [float(r[key]) for r in releases]
    if t < times[0]:
        return 0.0
    if t >= times[-1]:
        return values[-1]
    for i in range(1, len(times)):
        if t <= times[i]:
            span = (times[i] - times[i - 1]).total_seconds()
            if span <= 0:
                return values[i]
            alpha = (t - times[i - 1]).total_seconds() / span
            return values[i - 1] + alpha * (values[i] - values[i - 1])
    return values[-1]


def quarter_deltas(releases: list[dict]) -> list[dict]:
    """Quarter increment = interpolated cumulative(end) − cumulative(start)."""
    t_first = parse_iso(releases[0]["published_at"])
    t_last = parse_iso(releases[-1]["published_at"])
    buckets: list[dict] = []
    d = quarter_start(t_first.date())
    while d <= t_last.date():
        end = add_months(d, 3)
        t_a = at_utc_midnight(d)
        t_b = at_utc_midnight(end)
        q = (d.month - 1) // 3 + 1
        row = {
            "start": d.isoformat(),
            "end": end.isoformat(),
            "label": f"Q{q}'{d.year % 100:02d}",
        }
        for src, dst in (
            ("total_features", "features"),
            ("total_external_prs", "external_prs"),
            ("total_commits", "commits"),
        ):
            a = interp_cumulative(releases, src, t_a)
            b = interp_cumulative(releases, src, t_b)
            row[dst] = round(b - a, 2)
            row[f"{dst}_at_start"] = round(a, 2)
            row[f"{dst}_at_end"] = round(b, 2)
        buckets.append(row)
        d = end
    return buckets


def time_marks(t0: date, t1: date) -> tuple[list[tuple[str, str]], list[tuple[str, str]]]:
    years: list[tuple[str, str]] = []
    quarters: list[tuple[str, str]] = []
    y, m = t0.year, ((t0.month - 1) // 3) * 3 + 1
    d = date(y, m, 1)
    while d <= t0:
        m += 3
        if m > 12:
            y, m = y + 1, m - 12
        d = date(y, m, 1)
    while d < t1:
        iso = d.isoformat()
        if d.month == 1:
            years.append((iso, str(d.year)))
        else:
            quarters.append((iso, f"Q{(d.month - 1) // 3 + 1}"))
        m += 3
        if m > 12:
            y, m = y + 1, m - 12
        d = date(y, m, 1)
    return years, quarters


def nice_max(v: float) -> float:
    v = max(float(v), 1.0)
    mag = 10 ** (len(str(int(v))) - 1)
    for m in (1, 2, 2.5, 5, 10):
        cand = mag * m
        if cand >= v:
            return float(cand)
    return float(v)


def yticks(vmax: float, n: int = 4) -> list[float]:
    return [vmax * i / n for i in range(n + 1)]


def fmt_tick(t: float) -> str:
    if abs(t - round(t)) < 1e-6:
        return str(int(round(t)))
    return f"{t:g}"


def write_svg(releases: list[dict], quarters: list[dict], path: Path) -> None:
    t0 = at_utc_midnight(date.fromisoformat(quarters[0]["start"]))
    t1 = parse_iso(releases[-1]["published_at"])
    span = (t1 - t0).total_seconds() or 1

    def frac(iso: str) -> float:
        dt = datetime.fromisoformat(iso).replace(tzinfo=timezone.utc)
        return min(1.0, max(0.0, (dt - t0).total_seconds() / span))

    year_marks, quarter_marks = time_marks(t0.date(), t1.date())
    w, h = 1140, 520
    ml, mr = 64, 58
    plot_w = w - ml - mr
    plot_h = 300

    def polyline(xs: list[float], ys: list[float], color: str) -> str:
        pts = " ".join(f"{x:.1f},{y:.1f}" for x, y in zip(xs, ys))
        dots = "".join(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="3.2" fill="{color}"/>' for x, y in zip(xs, ys))
        return f'<polyline fill="none" stroke="{color}" stroke-width="2" points="{pts}"/>{dots}'

    def guides(x0: float, y0: float) -> str:
        parts = []
        for iso, lab in quarter_marks:
            x = x0 + frac(iso) * plot_w
            parts.append(
                f'<line x1="{x:.1f}" y1="{y0}" x2="{x:.1f}" y2="{y0+plot_h}" stroke="#e5e7eb" stroke-dasharray="2 4"/>'
            )
            parts.append(
                f'<text x="{x+3:.1f}" y="{y0+12}" font-family="ui-sans-serif,system-ui,sans-serif" font-size="9" fill="#999">{lab}</text>'
            )
        for iso, lab in year_marks:
            x = x0 + frac(iso) * plot_w
            parts.append(
                f'<line x1="{x:.1f}" y1="{y0}" x2="{x:.1f}" y2="{y0+plot_h}" stroke="#bbb" stroke-dasharray="3 3"/>'
            )
            parts.append(
                f'<text x="{x+4:.1f}" y="{y0+12}" font-family="ui-sans-serif,system-ui,sans-serif" font-size="10" fill="#555">{lab}</text>'
            )
        return "\n".join(parts)

    def panel(
        x0: float,
        y0: float,
        title: str,
        series: list[tuple[str, list[float]]],
        ymax: float,
        xs_frac: list[float] | None = None,
        labels: list[str] | None = None,
        right_series: list[tuple[str, list[float]]] | None = None,
        ymax_right: float | None = None,
        left_label: str = "Count",
        right_label: str | None = None,
    ) -> str:
        xs = [x0 + f * plot_w for f in (xs_frac or [])]
        point_labels = labels or []
        parts = [
            f'<text x="{x0}" y="{y0-18}" font-family="ui-sans-serif,system-ui,sans-serif" font-size="15" font-weight="600" fill="#1a1a1a">{title}</text>',
            f'<text x="{x0-48}" y="{y0+plot_h/2}" font-family="ui-sans-serif,system-ui,sans-serif" font-size="11" fill="#666" transform="rotate(-90 {x0-48},{y0+plot_h/2})">{left_label}</text>',
            f'<rect x="{x0}" y="{y0}" width="{plot_w}" height="{plot_h}" fill="#fff" stroke="#ddd"/>',
            guides(x0, y0),
        ]
        for t in yticks(ymax):
            y = y0 + plot_h - (t / ymax) * plot_h
            parts.append(f'<line x1="{x0}" y1="{y:.1f}" x2="{x0+plot_w}" y2="{y:.1f}" stroke="#eee"/>')
            parts.append(
                f'<text x="{x0-8}" y="{y+4:.1f}" text-anchor="end" font-family="ui-sans-serif,system-ui,sans-serif" font-size="10" fill="#666">{fmt_tick(t)}</text>'
            )
        if right_series and ymax_right:
            xr = x0 + plot_w
            for t in yticks(ymax_right):
                y = y0 + plot_h - (t / ymax_right) * plot_h
                parts.append(
                    f'<text x="{xr+8}" y="{y+4:.1f}" font-family="ui-sans-serif,system-ui,sans-serif" font-size="10" fill="#6b7280">{fmt_tick(t)}</text>'
                )
            if right_label:
                parts.append(
                    f'<text x="{xr+46}" y="{y0+plot_h/2}" font-family="ui-sans-serif,system-ui,sans-serif" font-size="11" fill="#6b7280" transform="rotate(90 {xr+46},{y0+plot_h/2})">{right_label}</text>'
                )
        for color, values in series:
            ys = [y0 + plot_h - (v / ymax) * plot_h for v in values]
            parts.append(polyline(xs, ys, color))
        if right_series and ymax_right:
            for color, values in right_series:
                ys = [y0 + plot_h - (v / ymax_right) * plot_h for v in values]
                parts.append(polyline(xs, ys, color))
        for i, lab in enumerate(point_labels):
            y = y0 + plot_h + (14 if i % 2 == 0 else 28)
            parts.append(
                f'<text x="{xs[i]:.1f}" y="{y}" text-anchor="middle" font-family="ui-sans-serif,system-ui,sans-serif" font-size="9" fill="#444">{lab}</text>'
            )
        return "\n".join(parts)

    y1 = 80
    q_left = nice_max(max(max(q["features"], q["external_prs"]) for q in quarters) or 1)
    q_right = nice_max(max(q["commits"] for q in quarters) or 1)
    p1 = panel(
        ml,
        y1,
        "Quarterly increment (interpolated cumulative at quarter end − start)",
        [
            ("#2563eb", [q["features"] for q in quarters]),
            ("#16a34a", [q["external_prs"] for q in quarters]),
        ],
        q_left,
        xs_frac=[frac(q["start"]) for q in quarters],
        labels=[q["label"] for q in quarters],
        right_series=[("#6b7280", [q["commits"] for q in quarters])],
        ymax_right=q_right,
        left_label="Features / PRs",
        right_label="Commits",
    )
    legend = []
    lx = ml
    ly = y1 + plot_h + 46
    for name, color in (
        ("Features and improvements", "#2563eb"),
        ("External PRs", "#16a34a"),
        ("Commits", "#6b7280"),
    ):
        legend.append(f'<rect x="{lx}" y="{ly}" width="10" height="10" fill="{color}"/>')
        legend.append(
            f'<text x="{lx+16}" y="{ly+9}" font-family="ui-sans-serif,system-ui,sans-serif" font-size="12" fill="#333">{name}</text>'
        )
        lx += 12 + 16 + len(name) * 7.2 + 24

    svg = f'''<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" viewBox="0 0 {w} {h}">
  <title>Userver Release Activity</title>
  <rect width="100%" height="100%" fill="#fafafa"/>
  <text x="{ml}" y="22" font-family="ui-sans-serif,system-ui,sans-serif" font-size="18" font-weight="700" fill="#111">Userver Release Activity</text>
  {p1}
  {''.join(legend)}
  <text x="{ml}" y="{h-12}" font-family="ui-sans-serif,system-ui,sans-serif" font-size="10" fill="#777">X = GitHub published_at ({releases[0]["published"]} … {releases[-1]["published"]}). Quarterly Y = interpolated cumulative at quarter end minus start (0 before first release). Sources: GitHub releases; roadmap_and_changelog.md; arc log.</text>
</svg>
'''
    path.write_text(svg, encoding="utf-8")



def main() -> None:
    ap = argparse.ArgumentParser(description="Build Userver Release Activity chart (SVG).")
    ap.add_argument("--out", type=Path, help="output directory (default: userver root)")
    args = ap.parse_args()

    root = userver_root()
    out = args.out.resolve() if args.out else root
    out.mkdir(parents=True, exist_ok=True)

    gh = fetch_github_releases()
    print(f"github releases >= v2.1: {len(gh)} ({gh[0]['tag']} … {gh[-1]['tag']})")

    cl = parse_changelog(root / CHANGELOG_REL)
    releases = []
    for row in gh:
        tag = row["tag"]
        cl_stats = cl.get(tag, {})
        rel = {
            "tag": tag,
            "published_at": row["published_at"],
            "features": cl_stats.get("features", 0),
            "external_prs": cl_stats.get("external_prs", 0),
        }
        releases.append(rel)

    fill_commits(root, releases)
    window_days(releases)
    add_totals(releases)
    quarters = quarter_deltas(releases)

    write_svg(releases, quarters, out / SVG_NAME)

    print(f"wrote {out / SVG_NAME}")
    t = {
        "features": sum(r["features"] for r in releases),
        "external_prs": sum(r["external_prs"] for r in releases),
        "commits": sum(r["commits"] for r in releases),
        "total_features": releases[-1]["total_features"],
        "latest": f"{releases[-1]['tag']} features={releases[-1]['features']} prs={releases[-1]['external_prs']} commits={releases[-1]['commits']}",
    }
    print(
        f"totals: features={t['features']} external_prs={t['external_prs']} "
        f"commits={t['commits']} total_features={t['total_features']}"
    )
    print("latest:", t["latest"])


if __name__ == "__main__":
    main()
