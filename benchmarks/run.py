#!/usr/bin/env python3
"""List, run and report on keysMultitude benchmarks."""

import argparse
from datetime import datetime, timezone
from pathlib import Path
import re
import subprocess

from reporting.generate_report import (btree_degree, generate_report, generate_report_from_samples,
                                       read_samples, split_case_name, sweep_size)
from reporting.plot_btree_tradeoff import prepare_btree_degree_report
from suite import benchmark_budget, expand_backends, load_suite, read_suite_config


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SUITE = ROOT / "benchmarks" / "suites" / "default.toml"
REPORTS = {"standard": None, "btree-degree": prepare_btree_degree_report}


def discover(binary):
    output = subprocess.check_output(
        [str(binary), "--benchmark_list_tests"], text=True
    )
    names = output.splitlines()
    if not names:
        raise ValueError(f"No benchmarks registered in {binary}")
    return {name: split_case_name(name) for name in names}


def select_cases(cases, workloads, backends, sizes=None, btree_degrees=None):
    for selected, available, label in (
        (workloads, {case[0] for case in cases.values()}, "workloads"),
        (backends, {case[1] for case in cases.values()}, "backends"),
        (btree_degrees, {degree for _, backend, _ in cases.values()
                         if (degree := btree_degree(backend)) is not None}, "B-tree degrees"),
        (sizes, {size for _, _, params in cases.values()
                 if (size := sweep_size(params)) is not None}, "sizes"),
    ):
        unknown = set(selected or ()) - available
        if unknown:
            raise ValueError(
                f"Unknown {label}: {', '.join(map(str, sorted(unknown)))}. "
                f"Available: {', '.join(map(str, sorted(available)))}"
            )

    if btree_degrees:
        backends = {f"btree{degree}" for degree in btree_degrees}
    elif not backends:
        backends = expand_backends(read_suite_config(DEFAULT_SUITE).get("backends"), cases)

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


def make_run_directory(output):
    run_id = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S.%fZ")
    output = (output or ROOT / "build" / "benchmark-results" / run_id).resolve()
    output.mkdir(parents=True, exist_ok=False)
    return output


def measure(binary, names, raw, repetitions, budget):
    benchmark_filter = "^(" + "|".join(re.escape(name) for name in names) + ")$"
    subprocess.run(
        [
            str(binary),
            f"--benchmark_filter={benchmark_filter}",
            f"--benchmark_repetitions={repetitions}",
            f"--benchmark_min_time={budget}",
            "--benchmark_enable_random_interleaving",
            f"--benchmark_out={raw}",
            "--benchmark_out_format=json",
        ],
        check=True,
    )
    return read_samples(raw, names)


def run_benchmarks(args, names):
    if args.repetitions < 1:
        raise ValueError("--repetitions must be at least 1")
    budget = benchmark_budget(args.iterations, args.min_time)
    output = make_run_directory(args.output)
    samples = measure(args.binary, names, output / "raw.json", args.repetitions, budget)
    return generate_report_from_samples(samples, output, plot=not args.no_plot)


def run_suite(args, cases):
    report, runs = load_suite(args.suite or DEFAULT_SUITE, cases)
    if report not in REPORTS:
        raise ValueError(f"Unknown report {report!r}. Available: {', '.join(REPORTS)}")
    prepare_report = REPORTS[report]
    render_report = prepare_report([cases[name] for run in runs for name in run.names]) if prepare_report else None

    output = make_run_directory(args.output)
    samples = {}
    for index, run in enumerate(runs, 1):
        samples.update(measure(args.binary, run.names, output / f"run-{index}.json",
                               run.repetitions, run.budget))
    summary = generate_report_from_samples(samples, output, plot=not args.no_plot)
    if render_report and not args.no_plot:
        graph = render_report(output)
        with summary.open("a", encoding="utf-8") as stream:
            stream.write(f"\n[Сводный график]({graph.name})\n")
        print(f"Plot: {graph}")
    return summary


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    binary = ROOT / "build" / "benchmarks" / "keys_multitude_benchmarks"
    if (binary.with_suffix(".exe")).is_file() and not binary.is_file():
        binary = binary.with_suffix(".exe")

    parser.add_argument("--suite", type=Path, help="Suite TOML file (default: benchmarks/suites/default.toml)")
    parser.add_argument("--binary", type=Path, default=binary)
    parser.add_argument("--output", type=Path, help="New suite output directory")
    parser.add_argument("--no-plot", action="store_true")
    commands = parser.add_subparsers(dest="command")
    list_command = commands.add_parser("list", help="Show registered workload/backend pairs")
    run_command = commands.add_parser("run", help="Run selected cases and generate a report")
    for command in (list_command, run_command):
        command.add_argument("--binary", type=Path, default=binary)

    run_command.add_argument("--workloads", nargs="+", help="Default: all workloads")
    backend_filter = run_command.add_mutually_exclusive_group()
    backend_filter.add_argument("--backends", nargs="+",
                                help="Default: backends from the default suite")
    backend_filter.add_argument("--btree-degrees", nargs="+", type=int, metavar="T",
                                help="Compare only B-tree backends with the selected degrees")
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
    if args.suite is not None and args.command is not None:
        parser.error("--suite cannot be combined with a subcommand")

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

            if args.command is None:
                summary = run_suite(args, cases)
            else:
                names = select_cases(cases, args.workloads, args.backends, args.sizes,
                                     args.btree_degrees)
                summary = run_benchmarks(args, names)
    except (OSError, ValueError, KeyError, subprocess.CalledProcessError) as error:
        parser.exit(1, f"error: {error}\n")

    print(f"Report: {summary.resolve()}")


if __name__ == "__main__":
    main()
