import os
from typing import List, Tuple
import pandas as pd

from florasat.statistics.utils import (
    Config,
    load_stats,
    plot_cdf,
)


def analyze_hopcounts(config: Config):
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
                # add to data
                named_dfs.append((alg, df))

                print(df["hops"].max())

            ########## Plot data ##########
            file_path = config.results_path.joinpath(cstl).joinpath(sim_name)
            os.makedirs(file_path, exist_ok=True)
            file_path = file_path.joinpath(f"hopcount.cdf.pdf")
            plot_cdf(named_dfs, "hops", file_path, "Hops", mean=True)
