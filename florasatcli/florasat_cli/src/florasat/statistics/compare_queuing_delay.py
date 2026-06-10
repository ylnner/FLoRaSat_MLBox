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
class Graph:
    alg: str
    color: str
    cstl: str
    df: pd.DataFrame
    box: go.Box


def compare_queuing_delay(config: Config):
    algs_handled_once = False
    alg_graphs: Dict[str, List[Graph]] = {}
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
            is_first_cstl = True
            for cstl in config.cstl:
                print(f"\t\t\tWorking on {cstl}...")

                # Load all runs
                df = load_stats(config, cstl, sim, alg)
                # Concat runs
                df = pd.concat(df)

                df["recorded"] = df["recorded"].round(3)

                df = (
                    df.groupby("recorded")["queueDelay"]
                    .mean()
                    .pipe(pd.DataFrame)
                    .reset_index()
                )

                # print(df)
                df["queueDelay"] = (df["queueDelay"] * 1000).round()

                df["cstl"] = cstl

                # plot_dfs.append((alg, df))
                violin = go.Box(
                    name=alg,
                    x=df["cstl"],
                    y=df["queueDelay"],
                    # mode="lines",
                    showlegend=(not algs_handled_once and is_first_cstl),
                    legendgroup=alg,
                    offsetgroup=alg,
                    line_color=color,
                    boxpoints=False,
                )
                violin = Graph(alg, color2, cstl, df, violin)
                if sim in alg_graphs:
                    alg_graphs[sim].append(violin)
                else:
                    alg_graphs[sim] = [violin]
                is_first_cstl = False
        algs_handled_once = True
    names = list(map(lambda x: x, alg_graphs))
    fig = make_subplots(
        rows=1,
        cols=len(names),
        subplot_titles=names,
        # vertical_spacing=0.01,
        # horizontal_spacing=0.03,
        shared_yaxes=True,
    )
    for id, (name, graphs) in enumerate(alg_graphs.items(), 1):
        print(f"Subplot (1,{id}) {name}:")
        for graph in graphs:
            print("\t", "Plot", graph.alg, "for", graph.cstl)
            fig.add_trace(graph.box, row=1, col=id)
            # mean = sc.df["packetloss"].mean()
            # fig.add_hline(
            #     mean,
            #     row=1,  # type: ignore
            #     col=id,  # type: ignore
            #     line_dash="dot",
            #     line_width=2,
            #     line_color=sc.color,
            #     annotation_text=f"{round(mean,1)}%",
            #     annotation_font_color=sc.color,
            #     annotation_font_size=15,
            #     annotation_position=pos,
            #     annotation_bgcolor="white",
            #     annotation_opacity=0.8,
            # )

    fig.update_annotations(font=dict(size=22))
    fig.update_layout(
        legend=dict(yanchor="top", y=0.99, xanchor="left", x=0.001),
        boxmode="group"
        # yaxis_range=[0, 100],
    )
    # fig.update_xaxes(tickvals=config.cstl)
    fig.update_yaxes(title_text="Queuing Delay[ms]")
    file_path = config.results_path
    os.makedirs(file_path, exist_ok=True)
    file_path = file_path.joinpath(f"queueing-delay.comparison.pdf")
    print("\t", f"Write plot to file {file_path}...")
    apply_default(fig, 20, mt=40, width=1200, height=400)
    fig.write_image(file_path, engine="kaleido")
