from dataclasses import dataclass, asdict
import os
import time
from typing import List
import pandas as pd
import plotly.graph_objects as go
from florasat_statistics import load_routes

from florasat.statistics.utils import (
    Config,
    apply_default,
    get_route_dump_file,
    load_simulation_paths,
)


@dataclass
class Groundstation:
    id: int
    lat: float
    lon: float
    alt: int


def map_reason(reason: int) -> str:
    match reason:
        case 2 | 10:
            return "TTL expired"
        case 3:
            return "Wrong dest."
        case 4 | 7:
            return "If. down"
        case 5:
            return "If. invalid"
        case 6:
            return "Unroutable"
        case 8 | 11:
            return "Queue overflow"
        case 9:
            return "Retry limit reached"
        case _:
            raise Exception(f"Unexpected reason {reason}")


def create_drop_heatmap(config: Config):
    for cstl in config.cstl:
        for sim_name in config.sim_name:
            for alg in config.algorithms:
                ground_stations: List = []
                seen_ids = set()
                # get groundstations that were involved in traffic
                run_dfs = []
                for run in range(0, config.runs):
                    (stats_fp, _, _) = load_simulation_paths(
                        config, cstl, sim_name, alg, run
                    )
                    df = pd.read_csv(stats_fp)
                    # load and process routes
                    # (_, file_path) = get_route_dump_file(
                    #     config, cstl, sim_name, alg, run
                    # )

                    # print("\t", "Load", file_path)
                    # routes = load_routes(str(file_path))

                    # print("\t", "Get ground stations...")
                    # for r in routes:
                    #     for h in r.hops:
                    #         if h.typ == "G" and h.id not in seen_ids:
                    #             seen_ids.add(h.id)
                    #             gs = Groundstation(h.id, h.lat, h.lon, h.alt)
                    #             ground_stations.append(asdict(gs))

                    # print("\t", "Fill dataframe with drop locations...")

                    # last_lats = list(map(lambda x: round(x.hops[-1].lat), routes))
                    # last_lons = list(map(lambda x: round(x.hops[-1].lon), routes))

                    # df["dropLat"] = last_lats
                    # df["dropLon"] = last_lons

                    run_dfs.append(df)

                df = pd.concat(run_dfs)

                # number of packets
                packets = df.shape[0] / config.runs

                df = df.loc[df["dropReason"] != 99]

                counts = (
                    df.groupby(["dropReason"])["size"]
                    .count()
                    .pipe(pd.DataFrame)
                    .rename(columns={"size": "count"})
                )
                # normalize for runs
                counts["count"] = counts["count"] / config.runs

                # df = (
                #     df.groupby(["dropLat", "dropLon"])["dropLat"]
                #     .agg("count")
                #     .pipe(pd.DataFrame)
                #     .rename(columns={"dropLat": "dropped"})
                # )
                # # normalize for runs
                # df["dropped"] = df["dropped"] / config.runs
                # print("\t", f"Create plot for {alg}...")
                # fig = go.Figure()

                # Add dropspots
                # for (lat, lon), dropped in df.itertuples():
                #     fig.add_trace(
                #         go.Scattergeo(
                #             lon=[lon],
                #             lat=[lat],
                #             mode="markers",
                #             showlegend=False,
                #             opacity=0.9,
                #             marker=dict(
                #                 size=dropped**(1/3),
                #                 symbol="circle",
                #                 color="red",
                #             ),
                #         )
                #     )

                # Add groundstations
                # gs_df = pd.DataFrame(ground_stations)
                # fig.add_trace(
                #     go.Scattergeo(
                #         lon=gs_df["lon"],
                #         lat=gs_df["lat"],
                #         textposition="bottom right",
                #         mode="markers",
                #         showlegend=False,
                #         marker=dict(
                #             size=5,
                #             symbol="circle",
                #             color="black",
                #         ),
                #     )
                # )

                # print(counts)

                # add table
                overall = 0
                for id, (reason, count) in enumerate(counts.itertuples()):
                    print(map_reason(reason), count)
                    overall += count
                print(overall)
                #     fig.add_annotation(
                #         text=f"{map_reason(reason)}: {round(count)}",
                #         xref="paper",
                #         yref="paper",
                #         x=0.0,
                #         y=0.17 + (id / 18),
                #         showarrow=False,
                #         font=dict(size=20, color="#000"),
                #     )

                # print("\t", "Write plot to file...")
                # file_path = config.results_path.joinpath(cstl).joinpath(sim_name)
                # os.makedirs(file_path, exist_ok=True)
                # file_path = file_path.joinpath(f"{alg}-dropspots.map.pdf")
                # apply_default(fig)
                # fig.update_layout(
                #     margin=dict(l=0, r=0, b=0, t=0),
                # )
                # fig.write_image(file_path, engine="kaleido")
