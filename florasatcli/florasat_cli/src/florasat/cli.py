import argparse
import pathlib

import florasat.config.command as config_command
import florasat.statistics.command as statistics_command
import florasat.scenario.command as scenario_command


def generate_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser("florasat")
    subparsers = parser.add_subparsers(
        title="subcommands", help="Valid subcommands", dest="command", required=True
    )
    config_command.generate_config_subparser(subparsers)
    statistics_command.generate_statistics_subparser(subparsers)
    scenario_command.generate_scenario_subparser(subparsers)
    return parser


def run_command(args):
    if args.command is None:
        raise RuntimeError("Should not have reached this point without a command...")
    match args.command:
        case "statistics":
            statistics_command.handle_run(args)
        case "config":
            config_command.handle_run(args)
        case "scenario":
            scenario_command.handle_run(args)
        case cmd:
            raise RuntimeError(f"Unrecognized command: {cmd}")
