#!/usr/bin/env python3
"""Plot the effect of B-tree minimum degree at a fixed set size."""

import argparse
import csv
from functools import partial
from math import isfinite
from pathlib import Path
from statistics import mean

if __package__:
    from .generate_report import INK, btree_degree, format_time_tick, style_axis, sweep_size
else:
    from generate_report import INK, btree_degree, format_time_tick, style_axis, sweep_size


RESULTS = Path(__file__).resolve().parents[1] / "results" / "btreeDegreeDependency"
OPERATIONS = {
    "insert": ("Вставка", "#D45113", "P"),
    "range_count": ("Подсчёт в диапазоне", "#A34F91", "^"),
    "select_rank": ("Выбор по рангу", "#0072B2", "o"),
    "count_less": ("Подсчёт меньших ключей", "#009E73", "s"),
}


def load_measurements(results, size):
    measurements = {}
    for operation in OPERATIONS:
        path = results / f"{operation}.csv"
        if not path.is_file():
            directory = "insert" if operation == "insert" else "queries"
            path = results / directory / path.name
        by_degree = {}
        with path.open(encoding="utf-8", newline="") as stream:
            for row in csv.DictReader(stream):
                if int(row["size"]) != size:
                    continue
                degree = int(row["min_degree"])
                timing = float(row["mean_ns_per_op"])
                if degree in by_degree or not isfinite(timing) or timing <= 0:
                    raise ValueError(f"{path}: invalid or duplicate measurement for t={degree}")
                by_degree[degree] = timing
        if not by_degree:
            raise ValueError(f"{path}: no measurements for N={size}")
        measurements[operation] = by_degree

    degrees = sorted(measurements["insert"])
    if any(sorted(timings) != degrees for timings in measurements.values()):
        raise ValueError("All four operations must have measurements for the same degrees")
    return degrees, measurements


def write_tradeoff_plot(output, degrees, measurements):
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from matplotlib.ticker import FuncFormatter, LogLocator, NullLocator

    mixed = {
        degree: mean(measurements[operation][degree] for operation in OPERATIONS)
        for degree in degrees
    }
    best = min(degrees, key=mixed.__getitem__)

    with plt.rc_context({"font.family": "DejaVu Sans", "text.color": INK,
                         "axes.labelcolor": INK}):
        fig, axis = plt.subplots(figsize=(11, 5))
        fig.subplots_adjust(left=0.1, right=0.97, top=0.81, bottom=0.15)
        style_axis(axis, "y")
        for operation, (label, color, marker) in OPERATIONS.items():
            axis.plot(degrees, [measurements[operation][degree] for degree in degrees],
                      color=color, marker=marker, linewidth=1.8, markersize=5, label=label)

        axis.plot(degrees, [mixed[degree] for degree in degrees], color=INK,
                  linestyle="--", linewidth=2.8, marker="D", markersize=6,
                  label="Расчётное среднее")
        axis.scatter(best, mixed[best], s=130, facecolor="none", edgecolor=INK,
                     linewidth=1.5, zorder=5)
        axis.annotate(f"t = {best}", xy=(best, mixed[best]), xytext=(0, 14),
                      textcoords="offset points", ha="center", va="bottom",
                      fontweight="bold", arrowprops={"arrowstyle": "->", "color": INK,
                                                     "shrinkB": 6})
        axis.set_xscale("log", base=2)
        axis.set_yscale("log")
        axis.set_xticks(degrees, [str(degree) for degree in degrees])
        axis.xaxis.set_minor_locator(NullLocator())
        axis.yaxis.set_major_locator(LogLocator(base=10, subs=(1, 2, 5)))
        axis.yaxis.set_minor_locator(NullLocator())
        axis.yaxis.set_major_formatter(FuncFormatter(lambda value, _: format_time_tick(value)))
        axis.set_xlabel("Минимальная степень B-дерева, t", labelpad=10)
        axis.set_ylabel("Время процессора на операцию (лог. шкала)", labelpad=10)
        axis.legend(loc="lower left", bbox_to_anchor=(-0.008, 1.02), ncols=5,
                    frameon=False, fontsize=9, columnspacing=1.2)
        fig.text(0.1, 0.96, "Выбор оптимальной степени B-дерева",
                 fontsize=21, fontweight="bold", va="top")

        output.parent.mkdir(parents=True, exist_ok=True)
        fig.savefig(output, dpi=170)
        plt.close(fig)
    return best, mixed[best]


def prepare_btree_degree_report(cases):
    sizes = {sweep_size(params) for _, _, params in cases}
    if len(sizes) != 1 or None in sizes:
        raise ValueError("The btree-degree report requires a single set size")
    degrees = {btree_degree(backend) for _, backend, _ in cases}
    if None in degrees or len(degrees) < 2:
        raise ValueError("The btree-degree report requires at least two B-tree degrees")
    expected = {(operation, degree) for operation in OPERATIONS for degree in degrees}
    actual = {(operation, btree_degree(backend)) for operation, backend, _ in cases}
    if actual != expected or len(cases) != len(expected):
        raise ValueError("The btree-degree report requires one case per operation and degree")

    return partial(write_btree_degree_report, size=next(iter(sizes)))


def write_btree_degree_report(results, *, size):
    degrees, measurements = load_measurements(results, size)
    graph = results / f"degreeTradeoff-{size}.png"
    write_tradeoff_plot(graph, degrees, measurements)
    return {"Сводный график": graph}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--size", type=int, default=1_000_000)
    parser.add_argument("--results", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    fixed_results = RESULTS / f"fixedSize{args.size}"
    results = args.results or (fixed_results if fixed_results.is_dir() else RESULTS)
    output = args.output or results / f"degreeTradeoff-{args.size}.png"

    try:
        degrees, measurements = load_measurements(results, args.size)
        best, runtime = write_tradeoff_plot(output, degrees, measurements)
    except (ImportError, OSError, ValueError, KeyError) as error:
        parser.exit(1, f"error: {error}\n")

    print(f"Best tested degree for equal operation counts: t={best}, {runtime:.2f} ns/op")
    print(output.resolve())


if __name__ == "__main__":
    main()
