from math import ceil
import os
import time
from typing import List, Tuple
import numpy as np
import pandas as pd
from plotly.subplots import make_subplots
import plotly.graph_objects as go

from florasat.statistics.utils import Config, apply_default, load_stats


def analyze_deliveryratio(config: Config):
    for cstl in config.cstl:
        for sim_name in config.sim_name:
            plot_dfs: List[Tuple[str, pd.DataFrame]] = []
            for alg in config.algorithms:
                print("\t", f"Working on {alg}/{cstl}/{sim_name}...")
                # Load all runs
                df = load_stats(config, cstl, sim_name, alg)
                # Concat runs
                df = pd.concat(df)

                print("\t", "Process data...")
                df["created"] = df["created"].round(1)
                df["recorded"] = df["recorded"].round()

                delivered = (
                    df.loc[df["dropReason"] == 99]
                    .groupby("recorded")["recorded"]
                    .count()
                    .pipe(pd.DataFrame)
                    .rename(columns={"recorded": "rcvd"})
                )

                dropped = (
                    df.loc[df["dropReason"] != 99]
                    .groupby("recorded")["recorded"]
                    .count()
                    .pipe(pd.DataFrame)
                    .rename(columns={"recorded": "dropped"})
                )

                df = delivered.join(dropped).reset_index()

                max_val = df["recorded"].max()

                float_list = []
                for i in range(0, ceil(max_val)):
                    for x in range(0, 10):
                        float_list.append(round(i + round(x, 1), 1))

                df = (
                    df.set_index("recorded")
                    .reindex(float_list, fill_value=0)
                    .reset_index()
                )

                df["rcvd"] = df["rcvd"] / config.runs
                df["dropped"] = df["dropped"] / config.runs

                df.fillna(0, inplace=True)
                df.replace(np.NaN, 0, inplace=True)

                df["deliveryratio"] = (df["rcvd"] / (df["rcvd"] + df["dropped"])) * 100

                df.fillna(100, inplace=True)
                df.replace(np.NaN, 100, inplace=True)

                print()

                plot_dfs.append((alg, df))

            ########## Plot data ##########
            print("\t", "Create plot...")
            fig = make_subplots()
            colors = ["#636efa", "#ef553b", "#2ca02c", "#00cc96"]
            positions = ["top left", "top right", "bottom left", "bottom right"]
            for name, df in plot_dfs:
                color = colors.pop(0)
                position = positions.pop(0)
                fig.add_trace(
                    go.Scatter(
                        name=name,
                        x=df.recorded,
                        y=df["deliveryratio"].ewm(span=3000, adjust=False).mean(),
                        mode="lines",
                        line=dict(color=color)
                    )
                )
                mean_val = df["deliveryratio"].mean()
                fig.add_hline(
                    mean_val,
                    line_dash="dot",
                    line_width=1,
                    line_color=color,
                    annotation_text=f"{round(mean_val, 3)}%",
                    annotation_font_color=color,
                    annotation_font_size=20,
                    annotation_position=position,
                )
            fig.update_traces(line=dict(width=2), marker=dict(size=2))
            fig.update_layout(
                legend=dict(yanchor="top", y=0.95, xanchor="left", x=0.05),
                yaxis_range=[0, 100],
            )
            fig.update_xaxes(title_text="Time (s)", nticks=10)
            fig.update_yaxes(title_text="Delivery Ratio [%]", dtick=10)
            file_path = config.results_path.joinpath(cstl).joinpath(sim_name)
            os.makedirs(file_path, exist_ok=True)
            file_path = file_path.joinpath(f"delivery.ratio.pdf")
            print("\t", "Write plot to file", file_path)
            apply_default(fig)
            fig.write_image(file_path, engine="kaleido")
