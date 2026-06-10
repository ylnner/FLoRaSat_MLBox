import itertools
from math import floor
import os
from typing import List, Tuple
import numpy as np
import pandas as pd
from florasat_statistics import load_sat_stats
from plotly.subplots import make_subplots
import plotly.graph_objects as go

from florasat.statistics.utils import (
    Config,
    apply_default,
    get_sats_dump_file,
    load_simulation_paths,
)


def analyze_queues(config: Config):
    for cstl in config.cstl:
        for sim_name in config.sim_name:
            plot_dfs: List[Tuple[str, pd.DataFrame]] = []
            for alg in config.algorithms:
                print("\t", f"Working on {alg}/{cstl}/{sim_name}")
                # Load all runs
                run_dfs: List[pd.DataFrame] = []
                for run in range(0, config.runs):
                    (stats_path, _, _) = load_simulation_paths(
                        config, cstl, sim_name, alg, run
                    )
                    (_, file_path) = get_sats_dump_file(
                        config, cstl, sim_name, alg, run
                    )

                    print("\t", "Load", file_path)
                    satellites = load_sat_stats(str(file_path))

                    df = pd.DataFrame(columns=["id", "timestamp", "queueSize"])

                    print("\t", "Preprocess-data")

                    for sat in satellites:
                        id = sat.sat_id
                        entries = [[id, entry.start, entry.qs] for entry in sat.entries]

                        df = pd.concat(
                            [pd.DataFrame(entries, columns=df.columns), df],
                            ignore_index=True,
                        )

                    df["timestamp"] = df["timestamp"].round(1)

                    df = (
                        df.groupby(["timestamp"])["queueSize"]
                        .mean()
                        .pipe(pd.DataFrame)
                        .reset_index()
                    )

                    run_dfs.append(df)

                # fill with prev numbers
                ## find max
                max_val = 0
                for df in run_dfs:
                    max_val = max(max_val, df["timestamp"].max())

                processed_dfs = []
                for df in run_dfs:
                    float_list = []
                    for i in range(0, floor(max_val) + 1):
                        for x in range(0, 10):
                            float_list.append(round(i + round(x / 10, 1), 1))

                    df = (
                        df.set_index("timestamp")
                        .reindex(float_list, fill_value=np.NaN)
                        .fillna(method="ffill")
                    )
                    # df = df.reindex(indicies, fill_value=np.NaN).fillna(method="ffill")
                    processed_dfs.append(df)

                df = pd.concat(processed_dfs)

                df = (
                    df.groupby(["timestamp"])["queueSize"]
                    .mean()
                    .pipe(pd.DataFrame)
                    .reset_index()
                )

                df = (
                    df.groupby(pd.cut(df["timestamp"], 250, right=True))["queueSize"]
                    .mean()
                    .reset_index()
                )

                entries = []
                for index, row in df.iterrows():
                    start = max(0.0, row.timestamp.left + 0.001)
                    end = row.timestamp.right
                    queue_size = row.queueSize

                    entries.append([start, queue_size])
                    entries.append([end, queue_size])

                df = pd.DataFrame(entries, columns=["timestamp", "queueSize"])

                plot_dfs.append((alg, df))

            ########## Plot data ##########
            print("\t", "Create plot...")
            fig = make_subplots()

            for name, df in plot_dfs:
                fig.add_trace(
                    go.Scatter(
                        name=name,
                        x=df["timestamp"],
                        y=df["queueSize"],
                    )
                )
            fig.update_traces(line=dict(width=1), marker=dict(size=3))
            fig.update_layout(
                legend=dict(yanchor="top", y=0.95, xanchor="left", x=0.05),
            )
            # fig.update_layout(barmode='stack')
            fig.update_xaxes(
                title_text="Time (s)",
                nticks=15,
                showgrid=True,
                ticks="outside",
            )
            fig.update_yaxes(title_text="Avg. queued packets", nticks=10)
            print("\t", "Write plot to file...")
            file_path = config.results_path.joinpath(cstl).joinpath(sim_name)
            os.makedirs(file_path, exist_ok=True)
            file_path = file_path.joinpath(f"queues.comparison.pdf")
            apply_default(fig, size=18)
            fig.write_image(file_path, engine="kaleido")
