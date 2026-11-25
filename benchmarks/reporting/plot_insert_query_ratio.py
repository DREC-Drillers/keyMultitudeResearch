"""Compare backend timing and winners across insert/query ratios."""

import csv
from functools import partial
from itertools import product

from .generate_report import (INK, backend_style, format_integer, format_time_tick,
                              insert_percentage, style_axis, sweep_size)


PROFILES = {
    "mixed_range_count": "Подсчёт в диапазоне",
    "mixed_select_rank": "Выбор по рангу",
    "mixed_count_less": "Подсчёт меньших ключей",
    "mixed_all_queries": "Все три типа запросов",
}


def prepare_insert_query_report(cases):
    sizes = {sweep_size(params) for _, _, params in cases}
    if len(sizes) != 1 or None in sizes:
        raise ValueError("The insert-query-ratio report requires one initial set size")
    actual = {(workload, backend, insert_percentage(params))
              for workload, backend, params in cases}
    backends = {backend for _, backend, _ in actual}
    percentages = {percent for _, _, percent in actual}
    if (len(backends) < 2 or None in percentages
            or actual != set(product(PROFILES, backends, percentages))
            or len(cases) != len(actual)):
        raise ValueError("The insert-query-ratio report requires the same ratios and backends for all profiles")
    return partial(write_insert_query_report, size=next(iter(sizes)),
                   percentages=sorted(percentages), backends=sorted(backends))


def write_insert_query_report(output, *, size, percentages, backends):
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from matplotlib.colors import ListedColormap
    from matplotlib.patches import Patch

    styles = [backend_style(backend) for backend in backends]
    cells = []
    timings = {}
    winners = output / "insertQueryWinners.csv"
    with winners.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.writer(stream)
        writer.writerow(["workload", "size", "insert_pct", "winner", "median_ns_per_op",
                         "runner_up", "runner_up_ns_per_op", "runner_up_gap_pct"])
        for workload in PROFILES:
            with (output / f"{workload}.csv").open(encoding="utf-8", newline="") as data:
                medians = {(int(row["insert_pct"]), row["backend"]): float(row["median_ns_per_op"])
                           for row in csv.DictReader(data)}
            timings[workload] = medians
            row = []
            for percent in percentages:
                ranked = sorted(backends, key=lambda backend: medians[percent, backend])
                winner, runner_up = ranked[:2]
                best, second = medians[percent, winner], medians[percent, runner_up]
                writer.writerow([workload, size, percent, winner, best, runner_up, second,
                                 (second / best - 1) * 100])
                row.append(backends.index(winner))
            cells.append(row)

    with plt.rc_context({"font.family": "DejaVu Sans", "text.color": INK}):
        fig, axis = plt.subplots(figsize=(13, 3.8))
        fig.subplots_adjust(left=0.25, right=0.98, top=0.65, bottom=0.2)
        axis.pcolormesh(cells, cmap=ListedColormap([color for _, color, _ in styles]),
                        vmin=-0.5, vmax=len(styles) - 0.5, edgecolors="white", linewidth=1)
        axis.set_ylim(len(PROFILES), 0)
        axis.set_yticks([i + 0.5 for i in range(len(PROFILES))], list(PROFILES.values()))
        axis.set_xticks([i + 0.5 for i in range(len(percentages))], percentages)
        axis.set_xlabel("Доля вставок, %", labelpad=8)
        axis.tick_params(length=0, pad=6)
        axis.set_frame_on(False)
        fig.text(0.025, 0.93, "Какой бэкенд быстрее при разной доле вставок",
                 fontsize=17, fontweight="bold")
        fig.text(0.025, 0.85, f"{format_integer(size)} ключей в начале · минимум медианного времени")
        fig.legend(handles=[Patch(color=color, label=label) for label, color, _ in styles],
                   loc="upper left", bbox_to_anchor=(0.02, 0.81), ncols=len(styles), frameon=False)
        graph = output / "insertQueryWinners.png"
        fig.savefig(graph, dpi=170)
        plt.close(fig)

    comparison = output / "insertQueryBackendComparison.png"
    write_backend_comparison(plt, comparison, size, percentages, backends, timings)
    return {
        "Диаграмма победителей": graph,
        "Сравнение времени бэкендов": comparison,
        "Таблица победителей и отрывов": winners,
    }


def write_backend_comparison(plt, output, size, percentages, backends, timings):
    from matplotlib.ticker import FuncFormatter, NullLocator

    shown_percentages = percentages[::2]
    if shown_percentages[-1] != percentages[-1]:
        shown_percentages.append(percentages[-1])

    with plt.rc_context({"font.family": "DejaVu Sans", "text.color": INK, "axes.labelcolor": INK}):
        fig, axes = plt.subplots(2, 2, figsize=(13, 8), sharex=True)
        fig.subplots_adjust(left=0.105, right=0.975, top=0.77, bottom=0.12,
                            hspace=0.34, wspace=0.22)
        for axis, (workload, title) in zip(axes.flat, PROFILES.items(), strict=True):
            style_axis(axis, "y")
            for backend in backends:
                label, color, marker = backend_style(backend)
                axis.plot(percentages, [timings[workload][percent, backend] for percent in percentages],
                          label=label, color=color, marker=marker, markersize=3.5, linewidth=1.6)
            axis.set_title(title, loc="left", fontweight="bold", fontsize=11, pad=9)
            axis.set_yscale("log")
            axis.yaxis.set_major_formatter(FuncFormatter(lambda value, _: format_time_tick(value)))
            axis.yaxis.set_minor_locator(NullLocator())
            axis.set_xticks(shown_percentages)
            axis.tick_params(labelbottom=True, labelsize=9)

        handles, labels = axes[0, 0].get_legend_handles_labels()
        fig.legend(handles, labels, loc="upper left", bbox_to_anchor=(0.10, 0.88),
                   ncols=len(backends), frameon=False)
        fig.text(0.105, 0.96, "Время операций при разной доле вставок",
                 fontsize=20, fontweight="bold", va="top")
        fig.text(0.105, 0.90,
                 f"{format_integer(size)} ключей в начале · каждая панель со своей шкалой времени",
                 fontsize=11)
        fig.supxlabel("Доля вставок, %", y=0.045)
        fig.supylabel("Медианное время процессора на операцию (лог. шкала)", x=0.025)
        fig.savefig(output, dpi=170)
        plt.close(fig)
