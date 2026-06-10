import datetime
import os
from typing import List
from florasat_statistics import load_routes, load_sat_stats
import numpy as np
import pandas as pd
from florasat.statistics.utils import (
    Config,
    apply_default,
    get_sats_dump_file,
    load_simulation_paths,
)
import plotly.express as px
import plotly.graph_objects as go


def paramstudy_datarate(config: Config):
    start = datetime.datetime(2019, 1, 1, 0, 0)
    fig_delay = go.Figure()
    fig_congestion = go.Figure()
    init_datarate = None
    relative_mean = None
    for sim_name in config.sim_name:
        datarate = int(sim_name.split("-")[-1])
        datarate = datarate / 1000000
        if init_datarate is None:
            init_datarate = datarate
        factor = datarate / init_datarate
        print(f"Working on datarate {datarate}; Which is {factor}x")
        sizes = []
        size_pds: List[pd.DataFrame] = []
        size_dfs: List[pd.DataFrame] = []
        for cstl in config.cstl:
            size = cstl.split("-")[1] + " sats"
            sizes.append(size)
            sim_pd = None
            sim_df = None
            for alg in config.algorithms:
                alg_pd = None
                alg_df = None
                for run in range(config.runs):
                    (stats_path, _, _) = load_simulation_paths(
                        config, cstl, sim_name, alg, run
                    )
                    (_, file_path) = get_sats_dump_file(
                        config, cstl, sim_name, alg, run
                    )
                    sats = load_sat_stats(str(file_path))
                    df = pd.DataFrame(columns=["id", "timestamp", "queueSize"])
                    for sat in sats:
                        id = sat.sat_id
                        entries = [[id, entry.start, entry.qs] for entry in sat.entries]

                        df = pd.concat(
                            [pd.DataFrame(entries, columns=df.columns), df],
                            ignore_index=True,
                        )

                    # df = df.groupby(["id", "timestamp"]).agg("mean")
                    df["timestamp"] = df["timestamp"] * 1000 * 1000
                    df["timestamp"] = df["timestamp"].astype("timedelta64[us]") + start  # type: ignore
                    df = df.set_index("timestamp").resample("100us").last().ffill()
                    df = df.reset_index()
                    df = df.groupby("id")["queueSize"].mean().pipe(pd.DataFrame)
                    df["queueSize"] = df["queueSize"] / factor
                    # print(df)
                    if alg_df is None:
                        alg_df = df
                    else:
                        alg_df = pd.concat([alg_df, df])
                        alg_df.reset_index()
                        # print(alg_df)

                    ############################## e2e delay ##############################
                    df = pd.read_csv(stats_path)
                    # df["distance"] = distances
                    df = df.loc[(df["dropReason"] == 99) & (df["type"] == "N")]

                    if alg_pd is None:
                        alg_pd = df
                    else:
                        alg_pd = pd.concat([alg_pd, df])
                        alg_pd.reset_index()

                ############################################################
                if alg_df is None:
                    raise Exception("alg_df is none but should not!")
                alg_df = alg_df.groupby("id").agg("mean")
                if sim_df is None:
                    sim_df = alg_df
                else:
                    sim_df = pd.concat([sim_df, alg_df])
                    sim_df.reset_index()

                ############################## e2e delay ##############################
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

                alg_pd = alg_pd[["pid", "e2e-delay"]]

                alg_pd = alg_pd.groupby("pid").agg("mean")

                if sim_pd is None:
                    sim_pd = alg_pd
                else:
                    sim_pd = pd.concat([sim_pd, alg_pd])
                    sim_pd.reset_index()
            ############################################################
            if sim_df is None:
                raise Exception("sim_df is none but should not!")
            sim_df = sim_df.groupby("id").mean()

            if relative_mean is None:
                relative_mean = 1 / sim_df["queueSize"].mean()
            
            sim_df["queueSize"] = sim_df["queueSize"] * relative_mean

            size_dfs.append(sim_df)

            ############################## e2e delay ##############################
            if sim_pd is None:
                raise Exception("sim_pd is none but should not!")
            sim_pd = sim_pd.groupby("pid").mean()
            size_pds.append(sim_pd)

        for metric, fig, dfs in [
            ("e2e-delay", fig_delay, size_pds),
            ("queueSize", fig_congestion, size_dfs),
        ]:
            q1 = []
            median = []
            q3 = []
            lowerfence = []
            upperfence = []
            mean = []

            for df in dfs:
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
                    name=datarate,
                )
            )

    fig_delay.update_layout(
        legend={
            "title_text": "Datarate[Mbps]",
            "orientation": "h",
            "yanchor": "top",
            "y": 1.13,
            "xanchor": "left",
            "x": 0.02,
        },
        boxgap=0.01,
        boxgroupgap=0.2,
        boxmode="group",
        xaxis_title="Number of satellites",
        yaxis_title="Packet Delay [ms]",
    )

    fig_delay.update_yaxes(range=[0, 190])

    file_path = config.results_path
    os.makedirs(file_path, exist_ok=True)
    file_path = file_path.joinpath(f"paramstudy-datarate-delays.pdf")
    print("\t", "Write plot to file", file_path)
    apply_default(fig_delay, height=600, width=800)
    fig_delay.write_image(file_path, engine="kaleido")

    fig_congestion.update_layout(
        legend={
            "title_text": "Datarate[Mbps]",
            "orientation": "h",
            "yanchor": "top",
            "y": 1.13,
            "xanchor": "left",
            "x": 0.02,
        },
        boxgap=0.01,
        boxgroupgap=0.2,
        boxmode="group",
        xaxis_title="Number of satellites",
        yaxis_title="Relative congestion",
    )

    # fig_congestion.update_yaxes(range=[0, 50000])

    file_path = config.results_path
    os.makedirs(file_path, exist_ok=True)
    file_path = file_path.joinpath(f"paramstudy-datarate-congestion.pdf")
    print("\t", "Write plot to file", file_path)
    apply_default(fig_congestion, height=600, width=800)
    fig_congestion.write_image(file_path, engine="kaleido")
