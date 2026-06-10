import os
import time
from typing import Dict, List, Tuple
import pandas as pd
import plotly.graph_objects as go

from florasat.statistics.utils import Config, apply_default, load_stats
from plotly.subplots import make_subplots


def compare_delays(config: Config):
    group_by = 1

    for sim_name in config.sim_name:
        fig = make_subplots(
            rows=1,
            cols=4,
            subplot_titles=("Queuing Delay [ms]", "Processing Delay [ms]", "Transmission Delay [ms]", "Propagation Delay [ms]")
        )
        alg_dfs: Dict[str, pd.DataFrame] = {}
        for alg in config.algorithms:
            dfs: List[pd.DataFrame] = []
            for cstl in config.cstl:
                print("\t", f"Working on {alg}/{cstl}/{sim_name}...")
                # Load all runs
                df = load_stats(config, cstl, sim_name, alg)
                # Concat runs
                df = pd.concat(df)
                # Filter for delivered and Normal packets
                df = df.loc[(df["dropReason"] == 99) & (df["type"] == "N")]

                df.drop(columns=["dropReason", "type"], inplace=True)

                df = (
                    df.groupby(["pid"])[
                        [
                            "queueDelay",
                            "procDelay",
                            "transDelay",
                            "propDelay",
                            "srcGs",
                            "dstGs",
                            "created",
                        ]
                    ]
                    .mean()
                    .pipe(pd.DataFrame)
                    .reset_index()
                )

                df["created"] = (df["created"] / group_by).round().astype(
                    int
                ) * group_by

                df = (
                    df.groupby(["srcGs", "dstGs", "created"])[
                        [
                            "queueDelay",
                            "procDelay",
                            "transDelay",
                            "propDelay",
                        ]
                    ]
                    .mean()
                    .pipe(pd.DataFrame)
                    .reset_index()
                )

                # convert delays into ms
                df["queueDelay"] = (df["queueDelay"] * 1000).round()
                df["procDelay"] = (df["procDelay"] * 1000).round()
                df["transDelay"] = (df["transDelay"] * 1000).round()
                df["propDelay"] = (df["propDelay"] * 1000).round()

                df["alg"] = alg
                df["cstl"] = cstl


                # add to data
                dfs.append(df)

            alg_dfs[alg] = pd.concat(dfs)

        colors = ["#636efa", "#ef553b", "#2ca02c", "#00cc96"]
        for id, alg in enumerate(alg_dfs):
            color = colors.pop(0)
            df = alg_dfs[alg]

            fig.add_trace(
                go.Box(
                    x=df["cstl"],
                    y=df["queueDelay"],
                    legendgroup=alg,
                    name=alg,
                    boxpoints=False,
                    line_color=color,
                    showlegend=True,
                ),
                row=1,
                col=1,
            )

            fig.add_trace(
                go.Box(
                    x=df["cstl"],
                    y=df["procDelay"],
                    legendgroup=alg,
                    name=alg,
                    boxpoints=False,
                    line_color=color,
                    showlegend=False,
                ),
                row=1,
                col=2,
            )

            fig.add_trace(
                go.Box(
                    x=df["cstl"],
                    y=df["transDelay"],
                    legendgroup=alg,
                    name=alg,
                    boxpoints=False,
                    line_color=color,
                    showlegend=False,
                ),
                row=1,
                col=3,
            )

            fig.add_trace(
                go.Box(
                    x=df["cstl"],
                    y=df["propDelay"],
                    legendgroup=alg,
                    name=alg,
                    boxpoints=False,
                    line_color=color,
                    showlegend=False,
                ),
                row=1,
                col=4,
            )

        #     print("Adding trace for", name)
        fig.update_layout(boxmode="group")

        fig.update_layout(
            legend=dict(orientation="h", yanchor="bottom", y=1.05, xanchor="right", x=1)
        )
        fig.update_annotations(font=dict(size=20))
        # # write plot to file
        file_path = config.results_path.joinpath(sim_name)
        os.makedirs(file_path, exist_ok=True)
        file_path = file_path.joinpath(f"compare-delay.pdf")
        print("\t", "Write plot to file", file_path)
        apply_default(fig, size=20)
        fig.update_layout(width=1200, boxgap=0.01)
        fig.write_image(file_path, engine="kaleido")

        # for delay in ["queueDelay", "procDelay", "transDelay", "propDelay"]:
        #     print(f"Create plot for {delay}...")
        #     fig = go.Figure()

        #     for name, df in named_dfs:
        #         fig.add_trace(
        #             go.Box(x=df[delay].values, boxpoints=False, name=name)
        #         )

        #     # generate plot
        #     fig.update_xaxes(title_text="Delay [ms]", nticks=20)
        #     fig.update_layout(showlegend=False)

        #     # write plot to file
        #     file_path = config.results_path.joinpath(cstl).joinpath(sim_name)
        #     os.makedirs(file_path, exist_ok=True)
        #     file_path = file_path.joinpath(f"compare-{delay}.pdf")
        #     print("\t", "Write plot to file", file_path)
        #     apply_default(fig)
        #     fig.write_image(file_path, engine="kaleido")
