from dataclasses import dataclass
from math import ceil
import os
import time
from typing import Dict, List, Tuple
import pandas as pd
import plotly.graph_objects as go

from florasat.statistics.utils import Config, apply_default, load_stats
from plotly.subplots import make_subplots


@dataclass
class Scatter:
    alg: str
    color: str
    df: pd.DataFrame
    scatter: go.Scatter


def compare_failure_scenarios(config: Config):
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
                # Load all runs
                df = load_stats(config, cstl, sim, alg)
                # Concat runs
                df = pd.concat(df)

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

                print(max_val)

                float_list = []
                for i in range(0, ceil(max_val)):
                    float_list.append(i)
                    # for x in range(0, 10):
                    #     float_list.append(round(i + round(x, 1), 1))

                df = (
                    df.set_index("recorded")
                    .reindex(float_list, fill_value=0)
                    .reset_index()
                )

                df = df.fillna(0)

                df["rcvd"] = df["rcvd"] / config.runs
                df["dropped"] = df["dropped"] / config.runs

                df["packetloss"] = (df["dropped"] / (df["rcvd"] + df["dropped"])) * 100

                df = df.fillna(0)
                # plot_dfs.append((alg, df))
                scatter = go.Scatter(
                    name=alg,
                    x=df.recorded,
                    y=df["packetloss"].ewm(span=100, adjust=False).mean(),
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
                mean = sc.df["packetloss"].mean()
                fig.add_hline(
                    mean,
                    row=1,  # type: ignore
                    col=id,  # type: ignore
                    line_dash="dot",
                    line_width=2,
                    line_color=sc.color,
                    annotation_text=f"{round(mean,1)}%",
                    annotation_font_color=sc.color,
                    annotation_font_size=15,
                    annotation_position=pos,
                    annotation_bgcolor="white",
                    annotation_opacity=0.8
                )

        fig.update_annotations(font=dict(size=22))
        fig.update_layout(
            legend=dict(yanchor="top", y=0.99, xanchor="left", x=0.001),
            yaxis_range=[0, 100],
        )
        fig.update_xaxes(title_text="Time (s)", nticks=10)
        fig.update_yaxes(title_text="Packetloss [%]", dtick=10)
        print("\t", "Write plot to file...")
        file_path = config.results_path.joinpath(cstl)
        os.makedirs(file_path, exist_ok=True)
        file_path = file_path.joinpath(f"packetloss.comparison.pdf")
        apply_default(fig, 20, mt=40, width=1200, height=400)
        fig.write_image(file_path, engine="kaleido")
