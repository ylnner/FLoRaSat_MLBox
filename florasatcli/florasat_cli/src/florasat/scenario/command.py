import os
from pathlib import Path
import random
from time import gmtime, strftime
from typing import Set, Tuple
from sortedcontainers import SortedSet
from florasat.statistics import utils

config_name = ".florasat_config.toml"


def generate_scenario_subparser(subparsers):
    scenario_parser = subparsers.add_parser(
        "scenario", help="Create config for FLoRaSat CLI"
    )

    config_subparsers = scenario_parser.add_subparsers(
        help="Commands for scenario", dest="subcommand", required=True
    )

    create_parser = config_subparsers.add_parser(
        "create", help="Creates a new scenario."
    )

    create_parser.add_argument("type")
    create_parser.add_argument("satcount", type=int)
    create_parser.add_argument("simtime", type=int)
    create_parser.add_argument("phases", type=int)
    create_parser.add_argument("lfprop", type=float)
    create_parser.add_argument("nfprop", type=float)
    create_parser.add_argument("lrprop", type=float)
    create_parser.add_argument("nrprop", type=float)
    create_parser.add_argument("--name", dest="name", type=str, required=False)
    create_parser.add_argument("--warmup", dest="warmup", type=int, required=False)
    create_parser.add_argument(
        "--seed", dest="seed", type=str, nargs="+", required=True
    )


def handle_run(args):
    match args.subcommand:
        case "create":
            __create(args)


def __create(args):
    match args.type:
        case "failures":
            __create_failures(args)


def __create_failures(args):
    
    seed = args.seed

    config = utils.load_config()

    name = args.name
    if name is None:
        name = strftime("%Y-%m-%d-%H:%M:%S", gmtime())

    for id, seed in enumerate(seed):
        text = create_content(args, seed)
        path = Path(config["results_path"]).joinpath("scenarios")
        file_path = path.joinpath(f"{name}{id}.xml")
        file_path.parent.mkdir(parents=True, exist_ok=True)
        print(f"Write scenario to {file_path}")
        with open(file_path, "w") as f:
            f.write(text)


def decision(probability: float):
    return random.random() < probability


def create_content(args, seed) -> str:
    print("Generate failures scenario...")

    random.seed(seed)

    satcount = args.satcount
    simtime = args.simtime
    phases = args.phases
    lfprop = args.lfprop
    nfprop = args.nfprop
    lrprop = args.lrprop
    nrprop = args.nrprop
    warmup = args.warmup

    if warmup is None:
        warmup = 0

    phase_length = (simtime - warmup) / phases

    working_links: Set[Tuple[int, str]] = SortedSet(
        (i, r) for r in ["LEFT", "UP", "RIGHT", "DOWN"] for i in range(0, satcount)
    )
    failed_links: Set[Tuple[int, str]] = SortedSet()
    working_nodes = SortedSet(i for i in range(0, satcount))
    failed_nodes = SortedSet()

    text = f"<!-- Sats: {satcount}; Time: {simtime}s; Phase: {phase_length}s; lfprob: {lfprop}; lrprob: {lrprop}; nfprob: {nfprop}; nrprob: {nrprop}; Warmup: {warmup}; Seed: {seed}  -->\n"
    text += "<scenario>\n"

    for phase in range(0, phases):
        start = phase * phase_length + warmup
        # end = (phase + 1) * phase_length + warmup
        # print(f"Generate phase {phase} [{start}-{end}]")

        ############### LINKS ###############

        # destroy new links
        num_working_links = len(working_links)
        new_failed_links = SortedSet()
        tmp_working_links = SortedSet()
        for i in range(0, num_working_links):
            wl = working_links.pop()
            if decision(lfprop):
                new_failed_links.add(wl)
            else:
                tmp_working_links.add(wl)
        assert len(working_links) == 0
        assert len(new_failed_links) + len(tmp_working_links) == num_working_links
        working_links = tmp_working_links

        # repair links
        num_failed_links = len(failed_links)
        repaired_links: Set[Tuple[int, str]] = SortedSet()
        tmp_failed_links = SortedSet()
        for i in range(0, num_failed_links):
            fl = failed_links.pop()
            if decision(lrprop):
                repaired_links.add(fl)
            else:
                tmp_failed_links.add(fl)
        assert len(failed_links) == 0
        assert len(repaired_links) + len(tmp_failed_links) == num_failed_links
        failed_links = tmp_failed_links

        working_links = working_links.union(repaired_links)
        failed_links = failed_links.union(new_failed_links)
        assert len(working_links) + len(failed_links) == satcount * 4

        ############### NODES ###############

        # destroy new nodes
        num_working_nodes = len(working_nodes)
        new_failed_nodes = SortedSet()
        tmp_working_nodes = SortedSet()
        for i in range(0, num_working_nodes):
            wl = working_nodes.pop()
            if decision(nfprop):
                new_failed_nodes.add(wl)
            else:
                tmp_working_nodes.add(wl)
        assert len(working_nodes) == 0
        assert len(new_failed_nodes) + len(tmp_working_nodes) == num_working_nodes
        working_nodes = tmp_working_nodes

        # repair nodes
        num_failed_nodes = len(failed_nodes)
        repaired_nodes: Set[Tuple[int, str]] = SortedSet()
        tmp_failed_nodes = SortedSet()
        for i in range(0, num_failed_nodes):
            fl = failed_nodes.pop()
            if decision(nrprop):
                repaired_nodes.add(fl)
            else:
                tmp_failed_nodes.add(fl)
        assert len(failed_nodes) == 0
        assert len(repaired_nodes) + len(tmp_failed_nodes) == num_failed_nodes
        failed_nodes = tmp_failed_nodes

        working_nodes = working_nodes.union(repaired_nodes)
        failed_nodes = failed_nodes.union(new_failed_nodes)
        assert len(working_nodes) + len(failed_nodes) == satcount

        # write as text

        text += f"\t<!-- LinkFailures: {len(failed_links)}; NodeFailures: {len(failed_nodes)} -->\n"
        text += f'\t<at t="{int(round(start))}s">\n'
        for id, dir in repaired_links:
            text += f'\t\t<set-isl-state satid="{id}" dir="{dir}" value="WORKING" />\n'
        for id, dir in new_failed_links:
            text += f'\t\t<set-isl-state satid="{id}" dir="{dir}" value="DISABLED" />\n'
        for repaired_node in repaired_nodes:
            text += f'\t\t<set-isl-state satid="{repaired_node}" value="WORKING" />\n'
        for failed_node in new_failed_nodes:
            text += f'\t\t<set-isl-state satid="{failed_node}" value="DISABLED" />\n'
        text += f"\t</at>\n"

    # clear up
    text += f"\t<!-- LinkFailures: 0; NodeFailures: 0 -->\n"
    text += f'\t<at t="{int(simtime)}s">\n'
    for id, dir in failed_links:
        text += f'\t\t<set-isl-state satid="{id}" dir="{dir}" value="WORKING" />\n'
    for repaired_node in failed_nodes:
        text += f'\t\t<set-isl-state satid="{repaired_node}" value="WORKING" />\n'
    text += f"\t</at>\n"
    text = text + "</scenario>\n"
    return text