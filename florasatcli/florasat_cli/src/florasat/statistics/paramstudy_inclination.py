import os
from typing import List
from florasat_statistics import load_routes
import numpy as np

import pandas as pd
from florasat.statistics.utils import Config, apply_default, get_route_dump_file, load_simulation_paths, load_stats
import plotly.express as px
import plotly.graph_objects as go

def paramstudy_inclination(config: Config):
    fig_distance = go.Figure()
    fig_delay = go.Figure()
    for sim_name in config.sim_name:
        incl = sim_name.split("-")[-1] + "°"
        print("Working on inclination", incl)
        sizes = []
        size_pds: List[pd.DataFrame] = []
        for cstl in config.cstl:
            size = cstl.split("-")[1] + " sats"
            sizes.append(size)
            sim_pd = None
            for alg in config.algorithms:
                alg_pd = None
                for run in range(config.runs):
                    (stats_path, _, _) = load_simulation_paths(
                        config, cstl, sim_name, alg, run
                    )
                    (_, file_path) = get_route_dump_file(
                        config, cstl, sim_name, alg, run
                    )

                    routes = load_routes(str(file_path))
                    distances = list(map(lambda r: r.length, routes))

                    df = pd.read_csv(stats_path)
                    df["distance"] = distances
                    df = df.loc[(df["dropReason"] == 99) & (df["type"] == "N")]

                    if alg_pd is None:
                        alg_pd = df
                    else:
                        alg_pd = pd.concat([alg_pd, df])
                        alg_pd.reset_index()
                if alg_pd is None:
                    raise Exception("Alg_pd is none but should not!")

                alg_pd["e2e-delay"] = (
                    (
                        alg_pd["queueDelay"]
                        + alg_pd["procDelay"]
                        + alg_pd["transDelay"]
                        + alg_pd["propDelay"]
                    )
                    * 1000
                ).round()

                alg_pd = alg_pd[["pid", "distance", "e2e-delay"]]

                alg_pd = alg_pd.groupby("pid").agg("mean")

                if sim_pd is None:
                    sim_pd = alg_pd
                else:
                    sim_pd = pd.concat([sim_pd, alg_pd])
                    sim_pd.reset_index()
            if sim_pd is None:
                raise Exception("sim_pd is none but should not!")

            sim_pd = sim_pd.groupby("pid").mean()
            print("Add", size)
            size_pds.append(sim_pd)

        for metric, fig in [("e2e-delay", fig_delay), ("distance", fig_distance)]:
            q1 = []
            median = []
            q3 = []
            lowerfence = []
            upperfence = []
            mean = []

            for df in size_pds:
                work_on = df[metric]
                tmp_q1 = np.percentile(work_on, 25)
                q1.append(tmp_q1)
                median.append(np.percentile(work_on, 50))  # median
                tmp_q3 = np.percentile(work_on, 75)
                q3.append(tmp_q3)  # q3

                iqr = tmp_q3 - tmp_q1
                th_upper = tmp_q3 + 1.5 * iqr
                max_delay = work_on.max()
                th_lower = tmp_q1 - 1.5 * iqr
                min_delay = work_on.min()

                lowerfence.append(max(min_delay, float(th_lower)))
                upperfence.append(min(max_delay, float(th_upper)))
                mean.append(work_on.mean())

            fig.add_trace(
                go.Box(
                    x=sizes,
                    y=[
                        [0],
                        [0],
                        [0],
                    ],
                    q1=q1,
                    median=median,
                    q3=q3,
                    lowerfence=lowerfence,
                    upperfence=upperfence,
                    mean=mean,
                    boxpoints=False,
                    name=incl,
                )
            )

    fig_delay.update_layout(
        legend={
            "title_text": "Inclination",
            "orientation": "h",
            "yanchor": "top",
            "y": 1.13,
            "xanchor": "left",
            "x": 0.02,
        },
        boxgap=0.023,
        boxgroupgap=0.2,
        boxmode="group",
        xaxis_title="Number of satellites",
        yaxis_title="Packet Delay [ms]",
    )

    fig_delay.update_yaxes(range=[0, 200])

    file_path = config.results_path
    os.makedirs(file_path, exist_ok=True)
    file_path = file_path.joinpath(f"paramstudy-inclination-delays.pdf")
    print("\t", "Write plot to file", file_path)
    apply_default(fig_delay, height=500, width=650)
    fig_delay.write_image(file_path, engine="kaleido")

    fig_distance.update_layout(
        legend={
            "title_text": "Inclination",
            "orientation": "h",
            "yanchor": "top",
            "y": 1.13,
            "xanchor": "left",
            "x": 0.02,
        },
        boxgap=0.02,
        boxgroupgap=0.2,
        boxmode="group",
        xaxis_title="Number of satellites",
        yaxis_title="Packet Distance [km]",
    )

    fig_distance.update_yaxes(range=[0, 50000])  

    file_path = config.results_path
    os.makedirs(file_path, exist_ok=True)
    file_path = file_path.joinpath(f"paramstudy-inclination-distances.pdf")
    print("\t", "Write plot to file", file_path)
    apply_default(fig_distance, height=500, width=650)
    fig_distance.write_image(file_path, engine="kaleido")

