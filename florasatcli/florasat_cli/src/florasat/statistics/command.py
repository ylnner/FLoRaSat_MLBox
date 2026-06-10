import os
from pathlib import Path
import sys
from typing import Any

import tomli
from florasat.statistics.analyze_distances import analyze_distances
from florasat.statistics.analyze_hopcount import analyze_hopcounts
from florasat.statistics.analyze_packetloss import analyze_packetloss
from florasat.statistics.analyze_deliveryratio import analyze_deliveryratio
from florasat.statistics.compare_delays import compare_delays
from florasat.statistics.preprocess_routes import preprocess_routes
from florasat.statistics.create_drop_heatmap import create_drop_heatmap
from florasat.statistics.analyze_e2edelay import analyze_e2edelay
from florasat.statistics import utils
from florasat.statistics.preprocess_satellites import preprocess_satellites
from florasat.statistics.analyze_queues import analyze_queues
from florasat.statistics.analyze_throughput import analyze_throughput
from florasat.statistics.paramstudy_altitude import paramstudy_altitude
from florasat.statistics.paramstudy_inclination import paramstudy_inclination
from florasat.statistics.paramstudy_datarate import paramstudy_datarate
from florasat.statistics.compare_failure_scenarios import compare_failure_scenarios
from florasat.statistics.compare_congestion_scenarios import (
    compare_congestion_scenarios,
)
from florasat.statistics.compare_queuing_delay import compare_queuing_delay


def generate_statistics_subparser(subparsers):
    stats_parser = subparsers.add_parser(
        "statistics", help="Create statistics for FLoRaSat"
    )

    stats_parser.add_argument(
        "--cstl",
        help="Constellation name from FLoRaSat",
        dest="cstl",
        type=str,
        nargs="+",
        required=True,
    )

    stats_parser.add_argument(
        "--name",
        help="Simulation names from FLoRaSat",
        dest="sim_name",
        type=str,
        nargs="+",
        required=True,
    )

    stats_parser.add_argument(
        "--algs",
        help="Algorithms for comparison.",
        dest="algs",
        type=str,
        nargs="+",
        required=True,
    )

    stats_parser.add_argument(
        "--runs",
        help="Number of simulation repeats",
        dest="runs",
        type=int,
        required=False,
    )

    stats_parser.add_argument(
        "--config",
        help="Path to config. Only required if config was not initialized at pre-defined location.",
        dest="config_path",
        type=str,
        required=False,
    )

    stats_parser.add_argument(
        "--flora",
        help="Path to read results written by FLoRaSat. If not specified, loaded from config.",
        dest="florasat_results_path",
        type=str,
        required=False,
    )

    stats_parser.add_argument(
        "--routes",
        help="Path to read/store pre-processed routes. If not specified, loaded from config.",
        dest="routes_path",
        type=str,
        required=False,
    )

    stats_parser.add_argument(
        "--satellites",
        help="Path to read/store pre-processed satellite states. If not specified, loaded from config.",
        dest="satellites_path",
        type=str,
        required=False,
    )

    stats_parser.add_argument(
        "--results",
        help="Path to read/store results. If not specified, loaded from config.",
        dest="results_path",
        type=str,
        required=False,
    )

    stats_parser.add_argument(
        "--preprocess-routes",
        help="Preprocess routes",
        dest="f_preprocess_routes",
        action="store_true",
        required=False,
    )

    stats_parser.add_argument(
        "--preprocess-satellites",
        help="Preprocess satellite states",
        dest="f_preprocess_satellites",
        action="store_true",
        required=False,
    )

    stats_parser.add_argument(
        "--hops",
        help="Generate hops CDF",
        dest="f_hops",
        action="store_true",
        required=False,
    )

    stats_parser.add_argument(
        "--distances",
        help="Generate distances CDF",
        dest="f_distances",
        action="store_true",
        required=False,
    )

    stats_parser.add_argument(
        "--packetloss",
        help="Generate packetloss graph",
        dest="f_packetloss",
        action="store_true",
        required=False,
    )
    
    stats_parser.add_argument(
        "--deliveryratio",
        help="Generate deliveryratio graph",
        dest="f_deliveryratio",
        action="store_true",
        required=False,
    )

    stats_parser.add_argument(
        "--drop-heatmap",
        help="Create packet drop heatmap",
        dest="f_drop_heatmap",
        action="store_true",
        required=False,
    )

    stats_parser.add_argument(
        "--e2e-delay",
        help="Generate E2E delay CDF",
        dest="f_e2e_delay_cdf",
        action="store_true",
        required=False,
    )

    stats_parser.add_argument(
        "--compare-delay",
        help="Generate delay comparison graph",
        dest="f_compare_delay",
        action="store_true",
        required=False,
    )

    stats_parser.add_argument(
        "--queue-sizes",
        help="Generate queue sizes comparison graph",
        dest="f_queue_sizes",
        action="store_true",
        required=False,
    )

    stats_parser.add_argument(
        "--throughput",
        help="Generate throughput comparison graph",
        dest="f_throughput",
        action="store_true",
        required=False,
    )

    stats_parser.add_argument(
        "--all",
        help="Generate all statistics",
        dest="f_all",
        action="store_true",
        required=False,
    )

    stats_parser.add_argument(
        "--compare-congestion-scenarios",
        help="Compares multiple congestion scenarios.",
        dest="f_compare_congestion",
        action="store_true",
        required=False,
    )

    stats_parser.add_argument(
        "--compare-failure-scenarios",
        help="Compares multiple failure scenarios.",
        dest="f_compare_failures",
        action="store_true",
        required=False,
    )

    stats_parser.add_argument(
        "--compare-queuing-delay",
        help="Compares queuing delay of multiple scenarios.",
        dest="f_compare_queuing_delays",
        action="store_true",
        required=False,
    )

    stats_parser.add_argument(
        "--paramstudy-altitude",
        help="Generate paramstudy altitude",
        dest="f_paramstudy_altitude",
        action="store_true",
        required=False,
    )

    stats_parser.add_argument(
        "--paramstudy-inclination",
        help="Generate paramstudy inclination",
        dest="f_paramstudy_inclination",
        action="store_true",
        required=False,
    )

    stats_parser.add_argument(
        "--paramstudy-datarate",
        help="Generate paramstudy datarate",
        dest="f_paramstudy_datarate",
        action="store_true",
        required=False,
    )


def handle_run(args):
    # load config if required value was not set in CLI
    if (
        args.florasat_results_path is None
        or args.routes_path is None
        or args.satellites_path is None
        or args.results_path is None
        or args.runs is None
    ):
        config = utils.load_config(args.config_path)

        if args.florasat_results_path is None:
            try:
                path = Path(config["florasat_results_path"])
                if not path.is_dir():
                    os.makedirs(path, exist_ok=True)
                args.florasat_results_path = path
            except KeyError:
                print(
                    f"X Could not find a value for 'florasat_results_path' in config file."
                )
                sys.exit(1)
            except OSError as e:
                print(f"X Failed:", e)
                sys.exit(1)

        if args.routes_path is None:
            try:
                path = Path(config["routes_path"])
                if not path.is_dir():
                    os.makedirs(path, exist_ok=True)
                args.routes_path = path
            except KeyError:
                print(f"X Could not find a value for 'routes_path' in config file.")
                sys.exit(1)
            except OSError as e:
                print(f"X Failed:", e)
                sys.exit(1)

        if args.satellites_path is None:
            try:
                path = Path(config["satellites_path"])
                if not path.is_dir():
                    os.makedirs(path, exist_ok=True)
                args.satellites_path = path
            except KeyError:
                print(f"X Could not find a value for 'satellites_path' in config file.")
                sys.exit(1)
            except OSError as e:
                print(f"X Failed:", e)
                sys.exit(1)

        if args.results_path is None:
            try:
                path = Path(config["results_path"])
                if not path.is_dir():
                    os.makedirs(path, exist_ok=True)
                args.results_path = path
            except KeyError:
                print(f"X Could not find a value for 'results_path' in config file.")
                sys.exit(1)
            except OSError as e:
                print(f"X Failed:", e)
                sys.exit(1)

        if args.runs is None:
            try:
                runs = int(config["runs"])
                args.runs = runs
            except KeyError:
                print(f"X Could not find a value for 'runs' in config file.")
                sys.exit(1)
            except ValueError:
                print(f"X Value for 'runs' in config file is no valid integer number.")
                sys.exit(1)

    if args.f_all:
        args.f_hops = True
        args.f_distances = True
        args.f_packetloss = True
        args.f_deliveryratio = True
        args.f_drop_heatmap = True
        args.f_e2e_delay_cdf = True
        args.f_compare_delay = True
        args.f_queue_sizes = True
        args.f_throughput = True

    print("")

    print("Run with configuration:")
    print("-> Cstl:", "\t", "\t", "\t", args.cstl)
    print("-> Simulation name:", "\t", "\t", args.sim_name)
    print("-> Algorithms:", "\t", "\t", "\t", args.algs)
    print("-> Runs:", "\t", "\t", "\t", args.runs)
    print("-> FLoRaSat result path:", "\t", args.florasat_results_path)
    print("-> Routes path:", "\t", "\t", args.routes_path)
    print("-> Satellites path:", "\t", "\t", args.satellites_path)
    print("-> Results path:", "\t", "\t", args.results_path)
    print("-> Preprocess routes:", "\t", "\t", args.f_preprocess_routes)
    print("-> Preprocess satellites:", "\t", args.f_preprocess_satellites)
    print("-> Gen. hops CDF:", "\t", "\t", args.f_hops)
    print("-> Gen. distance CDF:", "\t", "\t", args.f_distances)
    print("-> Gen. packetloss graph:", "\t", args.f_packetloss)
    print("-> Gen. deliveryratio graph:", "\t", args.f_deliveryratio)
    print("-> Gen. drop heatmap:", "\t", "\t", args.f_drop_heatmap)
    print("-> Gen. E2E delay CDF:", "\t", "\t", args.f_e2e_delay_cdf)
    print("-> Gen. delay comparison graph:\t", args.f_compare_delay)
    print("-> Gen. queue sizes graph:\t", args.f_queue_sizes)
    print("-> Gen. throughput graph:\t", args.f_throughput)
    print("-> Gen. paramstudy altitude:\t", args.f_paramstudy_altitude)
    print("-> Gen. paramstudy inclination:\t", args.f_paramstudy_inclination)
    print("-> Gen. paramstudy datarate:\t", args.f_paramstudy_datarate)

    # Validation
    print("")
    if not args.runs > 0:
        print("X Failure: At least 1 run required...")
        sys.exit(1)

    if (
        not args.f_preprocess_routes
        and not args.f_preprocess_satellites
        and not args.f_hops
        and not args.f_distances
        and not args.f_packetloss
        and not args.f_deliveryratio
        and not args.f_drop_heatmap
        and not args.f_e2e_delay_cdf
        and not args.f_compare_delay
        and not args.f_queue_sizes
        and not args.f_throughput
        and not args.f_paramstudy_altitude
        and not args.f_paramstudy_inclination
        and not args.f_paramstudy_datarate
        and not args.f_compare_failures
        and not args.f_compare_congestion
        and not args.f_compare_queuing_delays
    ):
        print("")
        print("Nothing to do...")
        sys.exit(0)

    stats_config = utils.Config(
        args.algs,
        args.cstl,
        args.sim_name,
        args.runs,
        args.florasat_results_path,
        args.routes_path,
        args.satellites_path,
        args.results_path,
    )

    if args.f_preprocess_routes:
        print("")
        print("Preprocess routes...")
        try:
            preprocess_routes(stats_config)
        except FileNotFoundError as e:
            print("X Failed to preprocess routes. Could not find:", e.filename)
            sys.exit(1)

    if args.f_preprocess_satellites:
        print("")
        print("Preprocess satellites...")
        try:
            preprocess_satellites(stats_config)
        except FileNotFoundError as e:
            print("X Failed to preprocess satellites. Could not find:", e.filename)
            sys.exit(1)

    if args.f_hops:
        print("")
        print("Run Hops CDF generation...")
        try:
            analyze_hopcounts(stats_config)
        except FileNotFoundError as e:
            print("X Failed to generate hops CDF. Could not find:", e.filename)
            sys.exit(1)

    if args.f_distances:
        print("")
        print("Run Distance CDF generation...")
        try:
            analyze_distances(stats_config)
        except FileNotFoundError as e:
            print("X Failed to generate distance CDF. Could not find:", e.filename)
            print(
                "Are routes preprocessed? This is required once after FLoRaSat simulation runs."
            )
            sys.exit(1)

    if args.f_packetloss:
        print("")
        print("Run packetloss graph generation...")
        try:
            analyze_packetloss(stats_config)
        except FileNotFoundError as e:
            print("X Failed to generate packetloss graph. Could not find:", e.filename)
            sys.exit(1)


    if args.f_deliveryratio:
        print("")
        print("Run deliveryratio graph generation...")
        try:
            analyze_deliveryratio(stats_config)
        except FileNotFoundError as e:
            print("X Failed to generate deliveryratio graph. Could not find:", e.filename)
            sys.exit(1)

    if args.f_drop_heatmap:
        print("")
        print("Run drop heatmap generation...")
        try:
            create_drop_heatmap(stats_config)
        except FileNotFoundError as e:
            print("X Failed to generate drop heatmap. Could not find:", e.filename)
            print(
                "Are routes preprocessed? This is required once after FLoRaSat simulation runs."
            )
            sys.exit(1)

    if args.f_queue_sizes:
        print("")
        print("Run queue size graph generation...")
        try:
            analyze_queues(stats_config)
        except FileNotFoundError as e:
            print("X Failed to generate queue size graph. Could not find:", e.filename)
            print(
                "Are satellite states preprocessed? This is required once after FLoRaSat simulation runs."
            )
            sys.exit(1)

    if args.f_e2e_delay_cdf:
        print("")
        print("Run E2E delay CDF generation...")
        try:
            analyze_e2edelay(stats_config)
        except FileNotFoundError as e:
            print("X Failed to generate E2E delay CDF. Could not find:", e.filename)
            sys.exit(1)

    if args.f_compare_delay:
        print("")
        print("Run delay comparison graph generation...")
        try:
            compare_delays(stats_config)
        except FileNotFoundError as e:
            print(
                "X Failed to generate delay comparison graph. Could not find:",
                e.filename,
            )
            sys.exit(1)

    if args.f_throughput:
        print("")
        print("Run analyze throughput graph generation...")
        try:
            analyze_throughput(stats_config)
        except FileNotFoundError as e:
            print(
                "X Failed to generate analyze throughput graph. Could not find:",
                e.filename,
            )
            sys.exit(1)

    if args.f_paramstudy_altitude:
        print("")
        print("Run paramstudy altitude graph generation...")
        try:
            paramstudy_altitude(stats_config)
        except FileNotFoundError as e:
            print(
                "X Failed to generate paramstudy altitude. Could not find:", e.filename
            )
            print(
                "Are satellite states preprocessed? This is required once after FLoRaSat simulation runs."
            )
            sys.exit(1)

    if args.f_paramstudy_inclination:
        print("")
        print("Run paramstudy inclination graph generation...")
        try:
            paramstudy_inclination(stats_config)
        except FileNotFoundError as e:
            print(
                "X Failed to generate paramstudy inclination. Could not find:",
                e.filename,
            )
            print(
                "Are satellite states preprocessed? This is required once after FLoRaSat simulation runs."
            )
            sys.exit(1)

    if args.f_paramstudy_datarate:
        print("")
        print("Run paramstudy datarate graph generation...")
        try:
            paramstudy_datarate(stats_config)
        except FileNotFoundError as e:
            print(
                "X Failed to generate paramstudy datarate. Could not find:", e.filename
            )
            print(
                "Are satellite states preprocessed? This is required once after FLoRaSat simulation runs."
            )
            sys.exit(1)

    if args.f_compare_failures:
        print("")
        print("Run compare failures comparison graph generation...")
        try:
            compare_failure_scenarios(stats_config)
        except FileNotFoundError as e:
            print(
                "X Failed to generate compare failures graph. Could not find:",
                e.filename,
            )
            sys.exit(1)

    if args.f_compare_congestion:
        print("")
        print("Run compare congestion comparison graph generation...")
        try:
            compare_congestion_scenarios(stats_config)
        except FileNotFoundError as e:
            print(
                "X Failed to generate compare congestions graph. Could not find:",
                e.filename,
            )
            print(
                "Are satellite states preprocessed? This is required once after FLoRaSat simulation runs."
            )
            sys.exit(1)

    if args.f_compare_queuing_delays:
        print("")
        print("Run generate compare queuing delays graph generation...")
        try:
            compare_queuing_delay(stats_config)
        except FileNotFoundError as e:
            print(
                "X Failed to generate compare queuing delays graph. Could not find:",
                e.filename,
            )
            sys.exit(1)
