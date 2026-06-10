import argparse
from pathlib import Path

import tomli
from florasat.utils.dir_path import dir_path
import os

config_name = ".florasat_config.toml"

raw_config = """florasat_results_path = "~/omnetpp-6.0.1/samples/florasat/simulations/routing/results"
routes_path = "./routes"
results_path = "./results"
runs = 4
"""


def generate_config_subparser(subparsers):
    config_parser = subparsers.add_parser(
        "config", help="Create config for FLoRaSat CLI"
    )

    config_subparsers = config_parser.add_subparsers(
        help="Commands for config", dest="subcommand", required=True
    )

    init_parser = config_subparsers.add_parser(
        "init", help="Initializes config file at $XDG_CONFIG_HOME/ or $HOME/."
    )
    init_parser.add_argument(
        "--path",
        type=dir_path,
        help="Alternative directory path to store initialized configuration.",
        required=False,
    )
    init_parser.add_argument(
        "--force",
        help="Force overwrite of already existing config file.",
        dest="f_force",
        action="store_true",
        required=False,
    )


def handle_run(args):
    match args.subcommand:
        case "init":
            __init(args.path, args.f_force)


def __init(path_raw: str | None, force: bool):
    path = __get_config_path(path_raw)
    print("Initialize config...", path)

    if path.exists() and not force:
        print("Path already exists:", path)
        print("If overwriting of old config is desired, use --force!")
        return

    with open(path, "w") as file:
        print("Write to", path)
        file.write(raw_config)
    print("Created configuration!")


def __get_config_path(path_raw: str | None) -> Path:
    if path_raw is not None:
        return Path(path_raw)

    print("No path specified")
    print("Checking $XDG_CONFIG_HOME/")
    xdg_config_home = os.environ.get("XDG_CONFIG_HOME")

    if xdg_config_home is not None:
        return Path(xdg_config_home).joinpath(config_name)

    print("Checking $HOME/")
    home = os.environ.get("HOME")
    if home is not None:
        return Path(home).joinpath(config_name)

    raise argparse.ArgumentError(None, f"'{path_raw}' is not a valid directory path!")
