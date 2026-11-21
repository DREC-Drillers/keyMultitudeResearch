"""Load a benchmark suite and resolve its runs against the binary's catalog."""

from dataclasses import dataclass
from fnmatch import fnmatchcase
from itertools import product
from math import isfinite
import tomllib

from btree_degrees import read_btree_degrees
from reporting.generate_report import sweep_size


@dataclass(frozen=True)
class SuiteRun:
    names: tuple[str, ...]
    repetitions: int
    budget: str


def benchmark_budget(iterations, min_time):
    if iterations is not None:
        if type(iterations) is not int or iterations < 1:
            raise ValueError("iterations must be a positive integer")
        return f"{iterations}x"
    if type(min_time) not in (int, float) or not isfinite(min_time) or min_time <= 0:
        raise ValueError("min_time must be a positive finite number of seconds")
    return f"{min_time:g}s"


def check_fields(config, allowed):
    unknown = config.keys() - allowed
    if unknown:
        raise ValueError(f"Unknown fields: {', '.join(sorted(unknown))}")


def require_list(value, field, valid):
    if not isinstance(value, list) or not value or not all(valid(item) for item in value):
        raise ValueError(f"{field} must be a non-empty list of valid values")
    return value


def expand_backends(patterns, cases):
    require_list(patterns, "backends", lambda item: isinstance(item, str) and bool(item))
    available = {backend for _, backend, _ in cases.values()}
    backends = set()
    for pattern in patterns:
        matches = {backend for backend in available if fnmatchcase(backend, pattern)}
        if not matches:
            raise ValueError(f"No backends match {pattern!r}. Available: {', '.join(sorted(available))}")
        backends.update(matches)
    return backends


def plan_run(config, cases):
    workloads = require_list(config.get("workloads"), "workloads",
                             lambda item: isinstance(item, str) and bool(item))
    backends = expand_backends(config.get("backends"), cases)
    sizes = config.get("sizes")
    if sizes is not None:
        require_list(sizes, "sizes", lambda item: type(item) is int and item > 0)

    repetitions = config.get("repetitions", 5)
    if type(repetitions) is not int or repetitions < 1:
        raise ValueError("repetitions must be a positive integer")
    if ("iterations" in config) == ("min_time" in config):
        raise ValueError("Set exactly one of iterations or min_time")
    budget = benchmark_budget(config.get("iterations"), config.get("min_time"))

    requested = set(product(workloads, backends, sizes if sizes is not None else (None,)))
    matched = set()
    names = []
    for name, (workload, backend, params) in cases.items():
        selection = (workload, backend, sweep_size(params) if sizes is not None else None)
        if selection in requested:
            matched.add(selection)
            names.append(name)
    if missing := requested - matched:
        labels = [f"{workload}/{backend}" + (f"/size:{size}" if size is not None else "")
                  for workload, backend, size in sorted(missing)]
        raise ValueError(f"Missing benchmark cases: {', '.join(labels)}")

    return SuiteRun(tuple(names), repetitions, budget)


def read_suite_config(path):
    with path.open("rb") as stream:
        return tomllib.load(stream)


def load_suite(path, cases):
    config = read_suite_config(path)
    shared = {"backends", "sizes", "repetitions"}
    check_fields(config, shared | {"runs", "report", "btree_degrees"})
    report = config.get("report", "standard")
    if not isinstance(report, str):
        raise ValueError("report must be a string")
    runs = require_list(config.get("runs"), "runs", lambda item: isinstance(item, dict))
    defaults = {field: config[field] for field in shared if field in config}
    if "btree_degrees" in config:
        if "backends" in config:
            raise ValueError("Choose either btree_degrees or backends in the suite")
        defaults["backends"] = [f"btree{degree}" for degree in read_btree_degrees(config)]

    plans = []
    seen = set()
    for index, run in enumerate(runs, 1):
        try:
            check_fields(run, shared | {"workloads", "iterations", "min_time"})
            plan = plan_run(defaults | run, cases)
            if duplicates := seen.intersection(plan.names):
                raise ValueError(f"Cases already included in another run: {', '.join(sorted(duplicates))}")
        except ValueError as error:
            raise ValueError(f"{path}, run {index}: {error}") from error
        seen.update(plan.names)
        plans.append(plan)

    return report, plans
