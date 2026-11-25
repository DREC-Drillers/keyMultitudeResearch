"""Convert Google Benchmark JSON into per-workload tables and plots."""

from collections import defaultdict
import csv
import json
import math
import re
from statistics import mean, median


WORKLOAD_TITLES = {
    "insert": "Вставка нового ключа",
    "range_count": "Подсчёт ключей в диапазоне",
    "select_rank": "Выбор ключа по рангу",
    "count_less": "Подсчёт ключей меньше заданного",
    "lookup": "Как размер множества влияет на время поиска",
    "range_queries": "Вставки и запросы по диапазону",
    "rank": "Выбор ключа по рангу",
    "mixed_range_count": "Вставки и подсчёт в диапазоне",
    "mixed_select_rank": "Вставки и выбор по рангу",
    "mixed_count_less": "Вставки и подсчёт меньших ключей",
    "mixed_all_queries": "Вставки и все три типа запросов",
}
SWEEP_DESCRIPTIONS = {
    "insert": "Новые уникальные ключи · 16–256 вставок около начального размера",
    "range_count": "Запрос (l, r] · каждый интервал содержит около 1 % ключей",
    "select_rank": "Ранги от 1 до N · запросы по всему отсортированному множеству",
    "count_less": "Строгая граница < x · запросы по всему отсортированному множеству",
    "lookup": "Проверка наличия ключа · 50 % попаданий, 50 % промахов",
}

BACKEND_STYLES = {
    "vector": ("Вектор", "#0072B2", "o"),
    "set": ("std::set", "#009E73", "s"),
    "avl": ("AVL-дерево", "#A34F91", "^"),
    "rb": ("Красно-чёрное дерево", "#D45113", "P"),
}
INK = "#162638"
MUTED = "#536477"
GRID = "#DDE5EE"
PANEL = "#F8FAFD"


def backend_style(backend):
    degree = btree_degree(backend)
    if degree is not None:
        return f"B-дерево (t={degree})", "#C47A00", "D"
    return BACKEND_STYLES.get(backend, (backend, "#536477", "o"))


def btree_degree(backend):
    match = re.fullmatch(r"btree([0-9]+)", backend)
    return int(match[1]) if match else None


def format_integer(value):
    return f"{value:,}".replace(",", " ")


def format_decimal(value):
    return f"{value:,.2f}".replace(",", " ").replace(".", ",")


def format_time_tick(nanoseconds):
    for scale, unit in ((1e9, "с"), (1e6, "мс"), (1e3, "мкс"), (1, "нс")):
        if nanoseconds >= scale or unit == "нс":
            value = f"{nanoseconds / scale:g}".replace(".", ",")
            return f"{value} {unit}"


def split_case_name(name):
    match = re.fullmatch(r"([a-z0-9_]+)/([a-z0-9_]+)((?:/[^/]+)*)", name)
    if not match:
        raise ValueError(f"Invalid benchmark name: {name}")
    workload, backend, parameters = match.groups()
    return workload, backend, parameters.lstrip("/")


def case_label(workload, parameters):
    values = dict(part.split(":", 1) for part in parameters.split("/") if ":" in part)
    size_key = "initial_size" if workload == "range_queries" else "size"
    if {size_key, "insert_pct"} <= values.keys():
        insert = int(values["insert_pct"])
        size = format_integer(int(values[size_key]))
        return f"{size} ключей в начале · вставки / запросы: {insert} / {100 - insert} %"
    if workload == "rank" and "size" in values:
        return f"{format_integer(int(values['size']))} ключей"
    return parameters.replace("/", ", ") or "Параметры по умолчанию"


def sweep_size(parameters):
    match = re.search(r"(?:^|/)size:([1-9][0-9]*)(?:/|$)", parameters)
    return int(match[1]) if match else None


def insert_percentage(parameters):
    match = re.search(r"(?:^|/)insert_pct:([0-9]+)(?:/|$)", parameters)
    return int(match[1]) if match else None


def read_samples(raw, expected):
    groups = defaultdict(list)
    for row in json.loads(raw.read_text(encoding="utf-8"))["benchmarks"]:
        if row.get("error_occurred"):
            raise ValueError(f"{row['name']}: {row.get('error_message', 'benchmark failed')}")
        if row.get("run_type") == "aggregate":
            continue

        name = row.get("run_name") or row["name"]
        rate = row["items_per_second"]
        if not math.isfinite(rate) or rate <= 0:
            raise ValueError(f"{name}: invalid operations per second: {rate}")
        groups[name].append(rate)

    if expected is not None and set(groups) != set(expected):
        missing = sorted(set(expected) - set(groups))
        unexpected = sorted(set(groups) - set(expected))
        raise ValueError(f"Unexpected benchmark output. Missing: {missing}. Extra: {unexpected}")
    if not groups:
        raise ValueError("No benchmark samples found in JSON")
    return groups


def style_axis(axis, grid_axis):
    axis.set_facecolor(PANEL)
    axis.set_axisbelow(True)
    axis.grid(axis=grid_axis, color=GRID, linewidth=0.85)
    axis.tick_params(axis="both", which="both", length=0, pad=7, colors=MUTED)
    for spine in axis.spines.values():
        spine.set_visible(False)


def write_plot(path, title, workload, cases, *, size_sweep, degree_sweep):
    try:
        import matplotlib
    except ImportError as error:
        raise ValueError("Plots require matplotlib. Use nix develop or pass --no-plot.") from error

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    with plt.rc_context({"font.family": "DejaVu Sans", "text.color": INK,
                         "axes.labelcolor": INK, "axes.titlecolor": INK}):
        if size_sweep:
            write_size_plot(plt, path, title, workload, cases, degree_sweep=degree_sweep)
        else:
            write_comparison_plot(plt, path, title, workload, cases)


def write_comparison_plot(plt, path, title, workload, cases):
    fig, axes = plt.subplots(
        len(cases), 1, figsize=(9.2, 2.75 * len(cases) + 0.6),
        sharex=True, squeeze=False, layout="constrained"
    )
    for axis, (parameters, rows) in zip(axes[:, 0], sorted(cases.items())):
        style_axis(axis, "x")
        for position, row in enumerate(rows):
            center = row["median_ns_per_op"]
            low = row["min_ns_per_op"]
            high = row["max_ns_per_op"]
            _, color, marker = backend_style(row["backend"])
            axis.errorbar(center, position, xerr=[[center - low], [high - center]],
                          fmt=marker, color=color, capsize=3, markersize=6)
        axis.set_yticks(range(len(rows)), [backend_style(row["backend"])[0] for row in rows])
        axis.set_ylim(len(rows) - 0.5, -0.5)
        axis.set_title(case_label(workload, parameters), loc="left", fontsize=11, pad=9)
        axis.set_xscale("log")
    minimum = min(row["min_ns_per_op"] for rows in cases.values() for row in rows)
    maximum = max(row["max_ns_per_op"] for rows in cases.values() for row in rows)
    axes[-1, 0].set_xlim(minimum / 1.15, maximum * 1.15)
    axes[-1, 0].set_xlabel("Время процессора на операцию, нс (логарифмическая шкала)")
    fig.suptitle(title, fontweight="bold", fontsize=17)
    fig.savefig(path, dpi=170)
    plt.close(fig)


def write_size_plot(plt, path, title, workload, cases, *, degree_sweep):
    from matplotlib.ticker import FuncFormatter, LogLocator, MaxNLocator, NullLocator

    series = defaultdict(list)
    for parameters, rows in sorted(cases.items(), key=lambda case: sweep_size(case[0])):
        size = sweep_size(parameters)
        for row in rows:
            if degree_sweep:
                series[size].append((btree_degree(row["backend"]), row))
            else:
                series[row["backend"]].append((size, row))

    fig, axis = plt.subplots(figsize=(10.6, 5.9))
    fig.subplots_adjust(left=0.095, right=0.972, top=0.765, bottom=0.17)
    style_axis(axis, "y")
    sizes_style = [
        ("#0072B2", "o"), ("#009E73", "s"), ("#C47A00", "D"),
        ("#A34F91", "^"), ("#D45113", "P"), ("#536477", "v"), ("#46899A", "X"),
    ]
    for index, (series_key, samples) in enumerate(series.items()):
        samples.sort(key=lambda point: point[0])
        x_values = [value for value, _ in samples]
        averages = [row["mean_ns_per_op"] for _, row in samples]
        if degree_sweep:
            label = format_integer(series_key)
            color, marker = sizes_style[index % len(sizes_style)]
        else:
            label, color, marker = backend_style(series_key)
        axis.fill_between(
            x_values,
            [row["min_ns_per_op"] for _, row in samples],
            [row["max_ns_per_op"] for _, row in samples],
            color=color,
            alpha=0.11,
            linewidth=0,
        )
        axis.plot(x_values, averages, marker=marker, label=label, color=color,
                  linewidth=2.3, markersize=6, markeredgecolor="white", markeredgewidth=0.7)

    x_ticks = sorted({value for samples in series.values() for value, _ in samples})
    axis.set_xscale("log", base=2)
    axis.set_xticks(x_ticks, [format_integer(value) for value in x_ticks])
    axis.xaxis.set_minor_locator(NullLocator())
    time_values = [row["mean_ns_per_op"] for rows in cases.values() for row in rows]
    log_threshold = 10 if degree_sweep else 100
    log_time = max(time_values) / min(time_values) >= log_threshold
    if log_time:
        axis.set_yscale("log")
        axis.yaxis.set_major_locator(LogLocator(base=10))
        axis.yaxis.set_major_formatter(FuncFormatter(lambda value, _: format_time_tick(value)))
    else:
        axis.yaxis.set_major_locator(MaxNLocator(nbins=6, integer=True))
        axis.yaxis.set_major_formatter(FuncFormatter(lambda value, _: format_integer(int(value))))
        axis.set_ylim(bottom=0)
    x_label = ("Минимальная степень B-дерева, t (логарифмическая шкала)" if degree_sweep
               else "Размер множества, ключей (логарифмическая шкала)")
    axis.set_xlabel(x_label, labelpad=10)
    time_label = ("Время процессора на операцию (лог. шкала)" if log_time
                  else "Время процессора на операцию, нс")
    axis.set_ylabel(time_label, labelpad=10)
    fig.text(0.095, 0.951, title, fontsize=20, fontweight="bold", va="top")
    subtitle = ("B-дерево · каждая линия — число ключей в начальном множестве"
                if degree_sweep else SWEEP_DESCRIPTIONS.get(workload, "Один бэкенд — одна линия"))
    fig.text(0.095, 0.892, subtitle,
             fontsize=11, color=MUTED, va="top")
    handles, labels = axis.get_legend_handles_labels()
    legend = dict(zip(labels, handles))
    order = [label for label, _, _ in BACKEND_STYLES.values() if label in legend]
    order.extend(label for label in labels if label not in order)
    axis.legend([legend[label] for label in order], order, loc="lower left",
                bbox_to_anchor=(-0.008, 1.03), ncols=len(order), frameon=False,
                fontsize=10, columnspacing=1.5)
    fig.text(0.095, 0.045, "Точки — среднее по повторам. Полосы — минимум и максимум.",
             color=MUTED, fontsize=10)
    fig.savefig(path, dpi=170)
    plt.close(fig)


def generate_report(raw, output, *, plot, expected=None):
    return generate_report_from_samples(read_samples(raw, expected), output, plot=plot)


def generate_report_from_samples(samples, output, *, plot):
    workloads = defaultdict(lambda: defaultdict(list))
    for name, rates in samples.items():
        workload, backend, parameters = split_case_name(name)
        times = [1e9 / rate for rate in rates]
        row = {
            "case": parameters,
            "backend": backend,
            "samples": len(rates),
            "median_ns_per_op": median(times),
            "min_ns_per_op": min(times),
            "max_ns_per_op": max(times),
            "median_ops_per_second": median(rates),
        }
        row["mean_ns_per_op"] = mean(times)
        workloads[workload][parameters].append(row)

    output.mkdir(parents=True, exist_ok=True)
    markdown = [
        "# Результаты бенчмарков",
        "",
        "Меньше времени на операцию — лучше.",
    ]
    has_plots = False
    for workload, cases in sorted(workloads.items()):
        title = WORKLOAD_TITLES.get(workload, workload)
        # A single size dimension is enough to get a comparison curve automatically.
        sizes = [sweep_size(parameters) for parameters in cases]
        ratio_sweep = all(size is not None and insert_percentage(parameters) is not None
                          for parameters, size in zip(cases, sizes))
        size_sweep = all(size is not None for size in sizes) and len(set(sizes)) == len(sizes)
        size_sweep = size_sweep and not ratio_sweep
        backends = {row["backend"] for group in cases.values() for row in group}
        degrees = {btree_degree(backend) for backend in backends}
        degree_sweep = size_sweep and None not in degrees and len(degrees) > 1
        markdown.extend(["", f"## {title}", "", f"[CSV]({workload}.csv)"])
        rows = []
        if size_sweep:
            backends = sorted(backends, key=btree_degree if degree_sweep else None)
            columns = [f"t = {btree_degree(backend)}" if degree_sweep else backend_style(backend)[0]
                       for backend in backends]
            markdown.extend([
                "",
                SWEEP_DESCRIPTIONS.get(workload, "Сравнение по размерам множества."),
                "В таблице — среднее время процессора на операцию, нс.",
                "",
                "| Размер множества | " + " | ".join(columns) + " |",
                "|---:|" + "---:|" * len(backends),
            ])
            for parameters, case_rows in sorted(cases.items(), key=lambda case: sweep_size(case[0])):
                size = sweep_size(parameters)
                by_backend = {row["backend"]: row for row in case_rows}
                cells = [
                    format_decimal(by_backend[backend]["mean_ns_per_op"]) if backend in by_backend else "—"
                    for backend in backends
                ]
                markdown.append(f"| {format_integer(size)} | " + " | ".join(cells) + " |")
                for row in case_rows:
                    row["size"] = size
                    if degree_sweep:
                        row["min_degree"] = btree_degree(row["backend"])
                rows.extend(by_backend[backend] for backend in backends if backend in by_backend)
        else:
            for parameters, case_rows in sorted(cases.items(),
                                                key=lambda case: (insert_percentage(case[0]) or 0, case[0])):
                case_rows.sort(key=lambda row: row["median_ns_per_op"])
                if ratio_sweep:
                    for row in case_rows:
                        row["size"] = sweep_size(parameters)
                        row["insert_pct"] = insert_percentage(parameters)
                rows.extend(case_rows)
                markdown.extend([
                    "",
                    f"### {case_label(workload, parameters)}",
                    "",
                    "| Бэкенд | Медиана, нс/оп | Мин–макс, нс/оп | Медиана, оп/с | Повторы |",
                    "|---|---:|---:|---:|---:|",
                ])
                for row in case_rows:
                    markdown.append(
                        f"| {backend_style(row['backend'])[0]} | {format_decimal(row['median_ns_per_op'])} | "
                        f"{format_decimal(row['min_ns_per_op'])}–{format_decimal(row['max_ns_per_op'])} | "
                        f"{format_integer(round(row['median_ops_per_second']))} | {row['samples']} |"
                    )

        with (output / f"{workload}.csv").open("w", encoding="utf-8", newline="") as stream:
            writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
            writer.writeheader()
            writer.writerows(rows)

        if plot and not ratio_sweep:
            write_plot(output / f"{workload}.png", title, workload, cases,
                       size_sweep=size_sweep, degree_sweep=degree_sweep)
            markdown.extend(["", f"![{title}]({workload}.png)"])
            has_plots = True

    if has_plots:
        markdown.insert(3, "Графики показывают измеренное время и разброс повторов.")

    summary = output / "summary.md"
    summary.write_text("\n".join(markdown) + "\n", encoding="utf-8")
    return summary
