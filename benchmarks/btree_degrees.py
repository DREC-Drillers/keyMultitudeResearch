"""Validate the configured B-tree degrees and expose them to the build."""

import argparse
from pathlib import Path
import tomllib


def read_btree_degrees(config):
    degrees = config.get("btree_degrees")
    if not isinstance(degrees, list) or not degrees or any(
        type(degree) is not int or degree < 2 for degree in degrees
    ):
        raise ValueError("btree_degrees must be a non-empty list of integers >= 2")
    if len(set(degrees)) != len(degrees):
        raise ValueError("btree_degrees must contain unique values")
    return degrees


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("config", type=Path)
    args = parser.parse_args()

    try:
        with args.config.open("rb") as stream:
            degrees = read_btree_degrees(tomllib.load(stream))
    except (OSError, ValueError) as error:
        parser.exit(1, f"{args.config}: {error}\n")

    print(", ".join(map(str, degrees)))


if __name__ == "__main__":
    main()
