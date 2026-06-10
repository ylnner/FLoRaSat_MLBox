from florasat.statistics.utils import Config, get_sats_dump_file, load_simulation_paths
from florasat_statistics import process_sat_stats


def preprocess_satellites(config: Config):
    ########### load data ##########
    for cstl in config.cstl:
        for sim_name in config.sim_name:
            for alg in config.algorithms:
                print("\t", f"Preprocess satellites for {alg}/{cstl}/{sim_name}...")
                for run in range(0, config.runs):
                    # load and process routes
                    (_, _, sats_fp) = load_simulation_paths(
                        config, cstl, sim_name, alg, run
                    )
                    (path, file_path) = get_sats_dump_file(
                        config, cstl, sim_name, alg, run
                    )
                    print("\t", "\t", "Read + Convert:", sats_fp)
                    print("\t", "\t", "-> Dump to:", file_path)
                    # Call Rust library function
                    process_sat_stats(str(sats_fp), str(file_path))
