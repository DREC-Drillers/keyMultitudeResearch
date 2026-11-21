#!/usr/bin/env python3
"""List, run and report on keysMultitude benchmarks."""

import argparse
from datetime import datetime, timezone
from pathlib import Path
import re
import subprocess

from reporting import generate_report, split_case_name, sweep_size


ROOT = Path(__file__).resolve().parents[1]


def discover(binary):
    output = subprocess.check_output(
        [str(binary), "--benchmark_list_tests"], text=True
    )
    names = output.splitlines()
    if not names:
        raise ValueError(f"No benchmarks registered in {binary}")
    return {name: split_case_name(name) for name in names}


def select_cases(cases, workloads, backends, sizes=None):
    for selected, available, label in (
        (workloads, {case[0] for case in cases.values()}, "workloads"),
        (backends, {case[1] for case in cases.values()}, "backends"),
        (sizes, {size for _, _, params in cases.values()
                 if (size := sweep_size(params)) is not None}, "sizes"),
    ):
        unknown = set(selected or ()) - available
        if unknown:
            raise ValueError(
                f"Unknown {label}: {', '.join(map(str, sorted(unknown)))}. "
                f"Available: {', '.join(map(str, sorted(available)))}"
            )

    names = [
        name
        for name, (workload, backend, params) in cases.items()
        if (not workloads or workload in workloads)
        and (not backends or backend in backends)
        and (not sizes or sweep_size(params) in sizes)
    ]
    if not names:
        raise ValueError("No benchmarks match the selected filters")
    return names


def run_benchmarks(args, names):
    if args.repetitions < 1:
        raise ValueError("--repetitions must be at least 1")
    if args.iterations is not None:
        if args.iterations < 1:
            raise ValueError("--iterations must be at least 1")
        benchmark_budget = f"{args.iterations}x"
    else:
        if not 0 < args.min_time < float("inf"):
            raise ValueError("--min-time must be a positive finite number of seconds")
        benchmark_budget = f"{args.min_time:g}s"

    run_id = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S.%fZ")
    output = (args.output or ROOT / "build" / "benchmark-results" / run_id).resolve()
    output.mkdir(parents=True, exist_ok=False)
    raw = output / "raw.json"

    benchmark_filter = "^(" + "|".join(re.escape(name) for name in names) + ")$"
    subprocess.run(
        [
            str(args.binary),
            f"--benchmark_filter={benchmark_filter}",
            f"--benchmark_repetitions={args.repetitions}",
            f"--benchmark_min_time={benchmark_budget}",
            "--benchmark_enable_random_interleaving",
            f"--benchmark_out={raw}",
            "--benchmark_out_format=json",
        ],
        check=True,
    )
    return generate_report(raw, output, plot=not args.no_plot, expected=names)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    binary = ROOT / "build" / "benchmarks" / "keys_multitude_benchmarks"
    if (binary.with_suffix(".exe")).is_file() and not binary.is_file():
        binary = binary.with_suffix(".exe")

    list_command = commands.add_parser("list", help="Show registered workload/backend pairs")
    run_command = commands.add_parser("run", help="Run selected cases and generate a report")
    for command in (list_command, run_command):
        command.add_argument("--binary", type=Path, default=binary)

    run_command.add_argument("--workloads", nargs="+", help="Default: all workloads")
    run_command.add_argument("--backends", nargs="+", help="Default: all backends")
    run_command.add_argument("--sizes", nargs="+", type=int,
                             help="Select registered set sizes. Default: all sizes")
    run_command.add_argument("--repetitions", type=int, default=5)
    budget = run_command.add_mutually_exclusive_group()
    budget.add_argument("--min-time", type=float, default=0.1, help="Seconds per case")
    budget.add_argument("--iterations", type=int, help="Fixed number of batches per repetition")
    run_command.add_argument("--output", type=Path, help="New output directory")
    run_command.add_argument("--no-plot", action="store_true")

    report_command = commands.add_parser("report", help="Regenerate a report from raw JSON")
    report_command.add_argument("raw", type=Path)
    report_command.add_argument("--output", type=Path, help="Defaults to the JSON directory")
    report_command.add_argument("--no-plot", action="store_true")
    args = parser.parse_args()

    try:
        if args.command == "report":
            summary = generate_report(
                args.raw, args.output or args.raw.parent, plot=not args.no_plot
            )
        else:
            args.binary = args.binary.resolve()
            if not args.binary.is_file():
                raise ValueError(
                    f"Benchmark binary not found: {args.binary}. "
                    "Build it with the benchmarks CMake preset or pass --binary."
                )
            cases = discover(args.binary)
            if args.command == "list":
                for workload in sorted({case[0] for case in cases.values()}):
                    backends = sorted(
                        {case[1] for case in cases.values() if case[0] == workload}
                    )
                    print(f"{workload}: {', '.join(backends)}")
                return

            names = select_cases(cases, args.workloads, args.backends, args.sizes)
            summary = run_benchmarks(args, names)
    except (OSError, ValueError, KeyError, subprocess.CalledProcessError) as error:
        parser.exit(1, f"error: {error}\n")

    print(f"Report: {summary.resolve()}")


if __name__ == "__main__":
    main()
