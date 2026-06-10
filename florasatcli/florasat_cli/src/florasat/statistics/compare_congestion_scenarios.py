from dataclasses import dataclass
import datetime
from functools import reduce
from math import ceil
import os
from pathlib import Path
import time
from typing import Dict, List, Tuple
from florasat_statistics import load_sat_stats
import numpy as np
import pandas as pd
import plotly.graph_objects as go

from florasat.statistics.utils import (
    Config,
    apply_default,
    get_sats_dump_file,
    load_stats,
)
from plotly.subplots import make_subplots


@dataclass
class Scatter:
    alg: str
    color: str
    df: pd.DataFrame
    scatter: go.Scatter


def compare_congestion_scenarios(config: Config):
    start = datetime.datetime(2019, 1, 1, 0, 0)
    for cstl in config.cstl:
        print(f"Working on {cstl}...")
        algs_handled_once = False
        scatters: Dict[str, List[Scatter]] = {}

        for sim in config.sim_name:
            print(f"\tWorking on scenario {sim}...")
            colors = [
                ("#636efa", "#6872f9"),
                ("#df553b", "#e06147"),
                ("#1ca02c", "#2b9f2b"),
                ("#00cc96", "#00cb95"),
            ]
            for alg in config.algorithms:
                (color, color2) = colors.pop(0)
                print(f"\t\tWorking on {alg}...")
                run_dfs: List[pd.DataFrame] = []

                tmp_path = Path(f"{cstl}-{sim}-{alg}")

                if tmp_path.exists():
                    df = pd.read_csv(tmp_path)
                    df = df.set_index("timestamp")
                else:
                    for run in range(config.runs):
                        print(f"\t\t\tWorking on run {run}...")
                        _, file_path = get_sats_dump_file(config, cstl, sim, alg, run)
                        print(f"\t\t\t\tLoad {run}...")
                        satellites = load_sat_stats(str(file_path))

                        print(f"\t\t\t\tPre-Process {run}...")
                        run_df = None
                        dfs = []
                        for id, sat in enumerate(satellites):
                            if id % 50 == 0:
                                print("Current sat:", id)
                            entries = [[0.0, 0]]
                            entries.extend(
                                [[entry.start, entry.qs] for entry in sat.entries]
                            )
                            entries.append([1500.0, 0])
                            df = pd.DataFrame(
                                entries,
                                columns=["timestamp", "queueSize"],
                            )

                            df["timestamp"] = df["timestamp"] * 1000 * 1000
                            df["timestamp"] = df["timestamp"].astype("timedelta64[us]") + start  # type: ignore
                            df = (
                                df.set_index("timestamp").resample("1ms").last().ffill()
                            )
                            df = df.resample("100ms").mean().ffill()
                            if run_df is None:
                                run_df = pd.DataFrame(
                                    [],
                                    columns=["timestamp"],
                                )
                                run_df["timestamp"] = df.index
                                run_df = run_df.set_index("timestamp")
                            run_df[id] = df["queueSize"]
                            # print(df["queueSize"].mean())
                            dfs.append(df)
                        assert run_df is not None
                        # dfs.insert(0, run_df)
                        # run_df = pd.concat(dfs, axis=1)
                        print(run_df)
                        run_dfs.append(run_df)

                    print("\t\t\t\tReduce {alg}...")
                    df = reduce(lambda a, b: a.add(b, fill_value=0), run_dfs)

                    print(f"\t\t\t\tCache {alg}...")
                    df.to_csv(tmp_path, index=True)
                df["mean"] = df.mean(axis=1)
                df["ts"] = pd.to_datetime(df.index.values).map(lambda x: x.second + x.minute * 60 + x.hour * 3600)
                df = df.groupby("ts").mean().pipe(pd.DataFrame)
                print(df)
                # print(df)

                # plot_dfs.append((alg, df))
                scatter = go.Scatter(
                    name=alg,
                    x=df.index,
                    y=df["mean"].ewm(span=20, adjust=False).mean(),
                    mode="lines",
                    showlegend=(not algs_handled_once),
                    legendgroup=alg,
                    line_color=color,
                )
                scatter = Scatter(alg, color2, df, scatter)
                if sim in scatters:
                    scatters[sim].append(scatter)
                else:
                    scatters[sim] = [scatter]
            algs_handled_once = True
        names = list(map(lambda x: x, scatters))
        fig = make_subplots(
            rows=1,
            cols=len(names),
            subplot_titles=names,
            vertical_spacing=0.01,
            horizontal_spacing=0.03,
            shared_yaxes=True,
        )
        for id, (name, scatter) in enumerate(scatters.items(), 1):
            positions = ["bottom left", "top right", "bottom right", "bottom left"]
            for sc in scatter:
                pos = positions.pop(0)
                fig.add_trace(sc.scatter, row=1, col=id)
                mean = sc.df["mean"].mean()
                fig.add_hline(
                    mean,
                    row=1,  # type: ignore
                    col=id,  # type: ignore
                    line_dash="dot",
                    line_width=3,
                    line_color="black",
                    annotation_text=f"{round(mean,1)}",
                    annotation_font_color=sc.color,
                    annotation_font_size=18,
                    annotation_position=pos,
                    annotation_bgcolor="white",
                )

        fig.update_annotations(font=dict(size=22))
        fig.update_layout(legend=dict(yanchor="top", y=0.99, xanchor="left", x=0.001))
        fig.update_xaxes(title_text="Time (s)", nticks=10)
        fig.update_yaxes(title_text="Avg. Queue Size", dtick=10)
        print("\t", "Write plot to file...")
        file_path = config.results_path.joinpath(cstl)
        os.makedirs(file_path, exist_ok=True)
        file_path = file_path.joinpath(f"congestion.comparison.pdf")
        apply_default(fig, 20, mt=40, width=1200, height=400)
        fig.write_image(file_path, engine="kaleido")
