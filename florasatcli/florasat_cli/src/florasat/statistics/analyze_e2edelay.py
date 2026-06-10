import os
from typing import List, Tuple
import pandas as pd

from florasat.statistics.utils import (
    Config,
    load_stats,
    plot_cdf,
)


def analyze_e2edelay(config: Config):
    for cstl in config.cstl:
        for sim_name in config.sim_name:
            named_dfs: List[Tuple[str, pd.DataFrame]] = []
            for alg in config.algorithms:
                print("\t", f"Working on {alg}/{cstl}/{sim_name}")
                # Load all runs
                df = load_stats(config, cstl, sim_name, alg)
                # Concat runs
                df = pd.concat(df)
                # Filter for delivered and Normal packets
                df = df.loc[(df["dropReason"] == 99) & (df["type"] == "N")]

                df["e2e-delay"] = (
                    (
                        df["queueDelay"]
                        + df["procDelay"]
                        + df["transDelay"]
                        + df["propDelay"]
                    )
                    * 1000
                ).round()

                # add to data
                named_dfs.append((alg, df))

            ########## Plot data ##########
            file_path = config.results_path.joinpath(cstl).joinpath(sim_name)
            os.makedirs(file_path, exist_ok=True)
            file_path = file_path.joinpath(f"e2e-delay.cdf.pdf")
            plot_cdf(named_dfs, "e2e-delay", file_path, "E2E Delay[ms]", mean=True, mean_unit="ms", percent_01_low=True, percent_1_low=True)
